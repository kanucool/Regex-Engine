#pragma once

#include <vector>
#include <string>
#include <array>
#include <stack>
#include <iostream>
#include <memory>
#include <unordered_set>

// data structures

enum class Type : char {
    LITERAL,
    CONCAT,
    STAR = '*',
    UNION = '|',
    DOT = '.',
    QUESTION = '?',
    PLUS = '+'
};

enum class Prec : char {
    LITERAL,
    PARENTHESES,
    LOW,
    MEDIUM,
    HIGH
};

struct Token {
    Type type;
    char c;
};

enum class NodeType : char {
    LITERAL,
    SPLIT,
    MATCH,
    WILDCARD
};

struct State {
    State* out[2] = {nullptr, nullptr};
    NodeType type;
    char c;

    State(NodeType type, char c = 0) : type(type), c(c) {}
};

struct Fragment {
    State* entry;
    std::vector<State**> exits;
};

class NFA {
private:
    std::vector<std::unique_ptr<State>> states;

public:
    State* start;

    State* makeState(NodeType type, char c = 0) {
        auto uPtr = std::make_unique<State>(type, c);
        State* s = uPtr.get();
        states.push_back(std::move(uPtr));
        return s;
    }

    void connect(Fragment& fragment, State* entry);
    void concatenate(Fragment& left, Fragment& right);
    State* postfixToNfa(std::vector<Token>& tokens);

    NFA() = default;

    NFA(std::vector<Token>& tokens) {
        start = postfixToNfa(tokens);
    }
};
    
constexpr std::array<Prec, 128> getPrecedenceArray() {
    std::array<Prec, 128> precedence = {};
    
    precedence[static_cast<char>(Type::UNION)] = Prec::LOW;
    precedence[static_cast<char>(Type::CONCAT)] = Prec::MEDIUM;
    precedence[static_cast<char>(Type::STAR)] = Prec::HIGH;
    precedence[static_cast<char>(Type::QUESTION)] = Prec::HIGH;
    precedence[static_cast<char>(Type::PLUS)] = Prec::HIGH;
    precedence['('] = precedence[')'] = Prec::PARENTHESES;

    return precedence;
}

constexpr auto PRECEDENCE = getPrecedenceArray();

// function declarations

std::vector<Token> regexToPostfix(std::string&);
bool simulateNfa(State*, std::string&);

