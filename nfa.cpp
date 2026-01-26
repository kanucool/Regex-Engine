#include "nfa.hpp"

bool canStart(uint8_t c) {
    return PRECEDENCE[c] == Prec::LITERAL || c == '(';
}

bool canEnd(uint8_t c) {
    if (PRECEDENCE[c] == Prec::LITERAL) return true;
    if (c == '*' || c == '?' || c == '.') return true;
    if (c == '+' || c == ')') return true;
    return false;
}

std::vector<Token> regexToPostfix(const std::string& expression) {

    std::vector<Token> res;
    std::stack<uint8_t, std::vector<uint8_t>> stk;

    auto push = [&stk, &res](uint8_t c) {
        Prec prec = PRECEDENCE[c];

        while (!stk.empty() && PRECEDENCE[stk.top()] >= prec) {
            uint8_t op = stk.top();
            res.push_back({static_cast<Type>(op), op});
            stk.pop();
        }
            
        stk.push(c);
    };

    auto pop = [&stk, &res]() {
        uint8_t op = stk.top();
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

    bool leftAnchor = !expression.empty() && expression[0] == '^';

    // if there is no ^ anchor, start with .*(
    if (!leftAnchor) {
        res.push_back({Type::DOT, '.'});
        res.push_back({Type::STAR, '*'});
        stk.push(static_cast<uint8_t>(Type::CONCAT));
        stk.push('(');
    }

    for (uint8_t c : expression) {
        // backslash and anchors
        if (!escaped && PRECEDENCE[c] == Prec::SPECIAL) {
            if (c == '\\') escaped = true;
            continue;
        } 
        
        // concat operator
        if ((canStart(c) || escaped) && prevCanEnd) {
            push(static_cast<uint8_t>(Type::CONCAT));
        }
        prevCanEnd = canEnd(c) || escaped;

        // literals
        if (escaped || PRECEDENCE[c] == Prec::LITERAL) {
            escaped = false;
            Type type = (c == '.') ? Type::DOT : Type::LITERAL;
            res.push_back({type, c});
        }

        // parentheses
        else if (PRECEDENCE[c] == Prec::PARENTHESES) {
            if (c == '(') stk.push(c);
            else rightParen();
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
    if (expression.empty() || expression.back() != '$') {
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

    for (auto [type, c] : tokens) {
        if (type == Type::LITERAL) {
            State* s = makeState(NodeType::LITERAL, c);
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
        else if (type == Type::DOT) {
            State* s = makeState(NodeType::WILDCARD);
            fragments.push({s, {s->out}});
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

bool simulateNfa(State* start, const std::string& candidate) {
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

    for (uint8_t c : candidate) {

        if (states.empty()) return false;
               
        expandSplits();
        std::swap(states, newStates);
        newStates.clear();

        for (State* state : states) {

            if ((state->type != NodeType::MATCH && state->c == c)
                    || state->type == NodeType::WILDCARD) {
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

