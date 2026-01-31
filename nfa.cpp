#include "nfa.hpp"

Prec getPrecedence(char_t c) {
    if (static_cast<uint64_t>(c) > 255) return Prec::LITERAL;
    return PRECEDENCE[c];
}

bool canStart(char_t c) {
    return getPrecedence(c) == Prec::LITERAL || c == '(' || c == '[';
}

bool canEnd(char_t c) {
    if (getPrecedence(c) == Prec::LITERAL) return true;
    if (c == '*' || c == '?' || c == '.') return true;
    if (c == '+' || c == ')' || c == ']') return true;
    return false;
}

bool escapedAtEnd(const string& expression) {
    if (expression.size() < 2) return false;

    uint64_t numSlashes = 0;
    int64_t idx = expression.size() - 2;

    while (idx >= 0 && expression[idx--] == '\\') numSlashes++;
    return numSlashes & 1;
}

void mergeIntervals(std::vector<ClassInterval>& intervals) {
    if (intervals.empty()) return;

    std::sort(intervals.begin(), intervals.end());
    uint64_t idx = 0;

    for (auto [l, r] : intervals) {
        char_t currR = intervals[idx].r;
        if (l > static_cast<uint64_t>(currR) + 1) {
            idx++;
            intervals[idx].l = l;
            intervals[idx].r = r;
        }
        else intervals[idx].r = std::max(currR, r);
    }

    intervals.resize(idx + 1);
}

std::vector<Token> regexToPostfix(string expression) {
    if (expression.empty()) return {};

    std::vector<Token> res;
    std::stack<char_t, std::vector<char_t>> stk;

    auto push = [&stk, &res](char_t c) {
        Prec prec = getPrecedence(c);

        while (!stk.empty() && getPrecedence(stk.top()) >= prec) {
            char_t op = stk.top();
            res.push_back({static_cast<Type>(op), op});
            stk.pop();
        }
            
        stk.push(c);
    };

    auto pop = [&stk, &res]() {
        char_t op = stk.top();
        if (op == ')') {
            throw std::runtime_error("unmatched (");
        } 
        res.push_back({static_cast<Type>(op), op});
        stk.pop();
    };

    auto rightParen = [pop, &stk]() {
        while (!stk.empty() && stk.top() != '(') pop();

        if (stk.empty()) {
            throw std::runtime_error("No matching (");
        }
        stk.pop();
    };

    bool escaped = false;
    bool prevCanEnd = false;
    bool inClass = false;
    bool hyphen = false;
    std::vector<ClassInterval> classSet;  

    bool leftAnchor = expression[0] == '^';
    bool rightAnchor = expression.back() == '$' && !escapedAtEnd(expression);

    if (rightAnchor) expression.pop_back();

    // if there is no ^ anchor, start with .*(
    if (!leftAnchor) {
        res.push_back({Type::DOT, '.'});
        res.push_back({Type::STAR, '*'});
        stk.push(static_cast<char_t>(Type::CONCAT));
        stk.push('(');
    }

    bool start = true;

    for (char_t c : expression) {
        // anchors
        if (c == '^' && start) {start = false; continue;}
        start = false;

        // backslash
        if (!escaped && getPrecedence(c) == Prec::SPECIAL) {
            if (c == '\\') escaped = true;
            continue;
        }

        // inside of a character class
        if (inClass && getPrecedence(c) != Prec::SQUARE_BRACKETS) {
            if (escaped || hyphen || c != '-' || classSet.empty()) {
                escaped = false;
                classSet.push_back({c, c});

                if (hyphen) {
                    hyphen = false;
                    auto [l1, r1] = classSet.back();
                    classSet.pop_back();
                    auto [l2, r2] = classSet.back();
                    classSet.pop_back();

                    char_t l = std::min(l1, l2);
                    char_t r = std::max(r1, r2);
                    classSet.push_back({l, r});
                }
            }
            else {
                if (hyphen) {
                    throw std::runtime_error("Double-hyphen in character class");
                }
                hyphen = true;
            }

            continue;
        }

        // concat operator
        if ((canStart(c) || escaped) && prevCanEnd) {
            push(static_cast<char_t>(Type::CONCAT));
        }
        prevCanEnd = canEnd(c) || escaped;

        // literals
        if (escaped || getPrecedence(c) == Prec::LITERAL) {
            escaped = false;
            Type type = (c == '.') ? Type::DOT : Type::LITERAL;
            res.push_back({type, c});
        }

        // parentheses
        else if (getPrecedence(c) == Prec::PARENTHESES) {
            if (c == '(') stk.push(c);
            else rightParen();
        }

        // character class
        else if (getPrecedence(c) == Prec::SQUARE_BRACKETS) {
            if (c == '[' && inClass) {
                throw std::runtime_error("Unexpected '[' in character class");
            }
            if (c == ']' && !inClass) {
                throw std::runtime_error("Unexpected ']' outside of character class");
            }
            
            if (c == '[') inClass = true;
            else {
                if (hyphen) {
                    throw std::runtime_error("Unmatched hyphen in character class");
                }
                
                mergeIntervals(classSet);
                res.push_back({Type::CLASS, '\0', std::move(classSet)});
                inClass = false;
                classSet.clear();
            }
        }
        
        // operators
        else push(c);
    }

    // if there is no left anchor, match the prepended (
    if (!leftAnchor) rightParen();

    while (!stk.empty()) {
        if (stk.top() == '(') {
            throw std::runtime_error("No matching )");
        }
        pop();
    }
    
    // if there is no right anchor $, append .*
    if (!rightAnchor) {
        res.push_back({Type::DOT, '.'}); 
        res.push_back({Type::STAR, '*'});
        if (prevCanEnd) {
            res.push_back({Type::CONCAT, ' '});
        }
    }

    return res;
}

void NFA::connect(Fragment& fragment, State* entry) {
  for (State** outPtr : fragment.exits) {
        *outPtr = entry;
    }
    fragment.exits.clear();
}

void NFA::concatenate(Fragment& left, Fragment& right) {
    State* entry = right.entry;
    connect(left, entry);
    left.exits = std::move(right.exits);
}

State* NFA::postfixToNfa(const std::vector<Token>& tokens) {
    std::stack<Fragment, std::vector<Fragment>> fragments;

    for (auto& [type, c, ranges] : tokens) {
        if (type == Type::LITERAL || type == Type::DOT) {
            NodeType nodeT = (type == Type::LITERAL) ?
                              NodeType::LITERAL :
                              NodeType::WILDCARD;
            State* s = makeState(nodeT, c);
            fragments.push({s, {s->out}}); 
        }
        else if (type == Type::CLASS) {
            State* s = makeState(NodeType::RANGES,
                                 '\0', std::move(ranges));
            fragments.push({s, {s->out}});  
        }
        else if (type == Type::CONCAT) {
            Fragment right = std::move(fragments.top());
            fragments.pop();
            concatenate(fragments.top(), right);
        }
        else if (type == Type::STAR) {
            State* s = makeState(NodeType::SPLIT);
            Fragment& fragment = fragments.top();
            s->out[0] = fragment.entry;

            while (!fragment.exits.empty()) {
                *(fragment.exits.back()) = s;
                fragment.exits.pop_back();
            }

            fragment.exits.push_back(&s->out[1]);
            fragment.entry = s;
        }
        else if (type == Type::UNION) {
            State* s = makeState(NodeType::SPLIT);

            Fragment a = std::move(fragments.top());
            fragments.pop();
            Fragment b = std::move(fragments.top());
            fragments.pop();

            s->out[0] = a.entry;
            s->out[1] = b.entry;

            std::vector<State**> exits = std::move(a.exits);
            exits.insert(exits.end(), b.exits.begin(),
                    b.exits.end());

            fragments.push({s, std::move(exits)});
        }
        else if (type == Type::QUESTION) {
            State* s = makeState(NodeType::SPLIT);
            Fragment& fragment = fragments.top();
            s->out[0] = fragment.entry;

            fragment.exits.push_back(&s->out[1]);
            fragment.entry = s;
        }
        else if (type == Type::PLUS) {
            State* s = makeState(NodeType::SPLIT);
            Fragment& fragment = fragments.top();

            connect(fragment, s);
            s->out[0] = fragment.entry;
            fragment.exits.push_back(&s->out[1]);
        }
    } 

    State* match = makeState(NodeType::MATCH);
    connect(fragments.top(), match);

    start = fragments.top().entry;
    return start;
}

bool searchRange(std::vector<ClassInterval>& ranges, char_t c) {
   if (!ranges.size()) return false;

    auto res = std::lower_bound(
        ranges.begin(),
        ranges.end(),
        static_cast<uint64_t>(c),
        [](const auto& obj, uint64_t c) {
            return obj.l < c;
        }
    );

    if (res == ranges.end() || res->l > c) {
        if (res == ranges.begin()) return false;
        --res;
    }

    return res->r >= c;
}

bool simulateNfa(State* start, const string& candidate) {
    if (!start) return candidate.empty();

    std::unordered_set<State*> states = {start};
    std::unordered_set<State*> newStates;
    std::stack<State*> splits;
    std::unordered_set<State*> visited;

    auto push = [&visited, &splits](State* state) {
        if (visited.find(state) == visited.end()) {
            visited.insert(state);
            splits.push(state);
        }
    };

    auto expandSplits = [&]() {
        newStates.clear();
        visited.clear();

        for (State* state : states) {
            if (state->type != NodeType::SPLIT) {
                newStates.insert(state);
            }
            else push(state);
        }

        while (!splits.empty()) {
            State* state = splits.top();
            splits.pop();
            
            if (state->type != NodeType::SPLIT) {
                newStates.insert(state);
            }
            else {
                push(state->out[0]);
                push(state->out[1]);
            }
        }
    };

    for (char_t c : candidate) {

        if (states.empty()) return false;
               
        expandSplits();
        std::swap(states, newStates);
        newStates.clear();

        for (State* state : states) {

            if ((state->type != NodeType::MATCH && state->c == c)
                    || state->type == NodeType::WILDCARD ||
                    (state->type == NodeType::RANGES &&
                    searchRange(state->ranges, c))) {
                newStates.insert(state->out[0]);
            }
        }

        std::swap(states, newStates);
        newStates.clear();
    }

    expandSplits();

    for (State* state : newStates) {
        if (state->type == NodeType::MATCH) {
            return true;
        }
    }

    return false;
}

std::vector<char32_t> convertToUtf32(const std::string& input) {
    std::vector<char32_t> res;
    uint8_t remainingBytes = 0;
    char32_t curr = 0;
    
    for (char c : input) {
        if (!remainingBytes) {
            if (!(c & 0x80)) {
                res.push_back(c);
                continue;
            }
            else if (!(c & 0x20)) {
                remainingBytes = 2;
                c &= 0x1F;;
            }
            else if (!(c & 0x10)) {
                remainingBytes = 3;
                c &= 0x0F;
            }
            else {
                remainingBytes = 4;
                c &= 0x07;
            }
        }
        
        curr = (curr << 6) | (c & 0x3F);

        if (!--remainingBytes) {
            res.push_back(curr);
            curr = 0;
        }
    }
    
    return res;
}

