#include "nfa.hpp"


bool canStart(char c) {
    if (PRECEDENCE[c] == Prec::LITERAL) return true;
    return c == '(';
}

bool canEnd(char c) {
    if (PRECEDENCE[c] == Prec::LITERAL) return true;
    if (c == '*' || c == '?' || c == '.') return true;
    if (c == '+' || c == ')') return true;
    return false;
}

std::vector<Token> regexToPostfix(std::string& expression) {

    std::vector<Token> res;
    std::stack<char, std::vector<char>> stk;

    auto push = [&stk, &res](char c) {
        Prec prec = PRECEDENCE[c];

        while (!stk.empty() && PRECEDENCE[stk.top()] >= prec) {
            char op = stk.top();
            res.push_back({static_cast<Type>(op), op});
            stk.pop();
        }
            
        stk.push(c);
    };

    auto pop = [&stk, &res]() {
        char op = stk.top();
        if (op == ')') {
            throw std::runtime_error("unmatched (");
        }
        res.push_back({static_cast<Type>(op), op});
        stk.pop();
    };

    bool escaped = false;
    bool prevCanEnd = false;

    for (char c : expression) {  
        // handle concat operator
        
        // an escape backslash is grouped with the next char
        if (c != '\\' || escaped) {
            if ((canStart(c) || escaped) && prevCanEnd) {
                push(static_cast<char>(Type::CONCAT));
            }
            prevCanEnd = canEnd(c) || escaped;
        }

        // handle literals and backslashes

        if (escaped || PRECEDENCE[c] == Prec::LITERAL) {
            if (c == '\\' && !escaped) {
                escaped = true;
            }
            else {
                escaped = false;
                res.push_back({Type::LITERAL, c});
            }
            continue;
        } 

        // handle parentheses and operators

        if (PRECEDENCE[c] == Prec::PARENTHESES) {
            if (c == '(') stk.push(c);
            else {
                while (!stk.empty() && stk.top() != '(') pop();

                if (stk.empty()) {
                    throw std::runtime_error("No matching (");
                }
                stk.pop();
            }
        }
        else {
            push(c);
        }
    }

    while (!stk.empty()) pop();
    return res;
}

int main() {
    std::string test; std::cin >> test;
    auto res = regexToPostfix(test);

    for (auto [type, c] : res) {
        if (type == Type::CONCAT) std::cout << '.';
        else std::cout << c;
    }
    std::cout << '\n';

    return 0;
}

