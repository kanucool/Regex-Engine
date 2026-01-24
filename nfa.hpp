#pragma once

#include <vector>
#include <string>
#include <array>
#include <stack>
#include <iostream>
#include <memory>
#include <unordered_set>

// data structures

enum class Type : uint8_t {
    LITERAL,
    CONCAT,
    STAR = '*',
    UNION = '|',
    DOT = '.',
    QUESTION = '?',
    PLUS = '+'
};

enum class Prec : uint8_t {
    LITERAL,
    PARENTHESES,
    LOW,
    MEDIUM,
    HIGH
};

struct Token {
    Type type;
    uint8_t c;
};

enum class NodeType : uint8_t {
    LITERAL,
    SPLIT,
    MATCH,
    WILDCARD
};

struct State {
    State* out[2] = {nullptr, nullptr};
    NodeType type;
    uint8_t c;

    State(NodeType type, uint8_t c = 0) : type(type), c(c) {}
};

struct Fragment {
    State* entry;
    std::vector<State**> exits;
};

class NFA {
private:
    std::vector<std::unique_ptr<State>> states;

public:
    State* start = nullptr;

    State* makeState(NodeType type, uint8_t c = 0) {
        auto uPtr = std::make_unique<State>(type, c);
        State* s = uPtr.get();
        states.push_back(std::move(uPtr));
        return s;
    }

    void connect(Fragment& fragment, State* entry);
    void concatenate(Fragment& left, Fragment& right);
    State* postfixToNfa(const std::vector<Token>& tokens);

    NFA() = default;

    NFA(const std::vector<Token>& tokens) {
        start = postfixToNfa(tokens);
    }
};
    
constexpr std::array<Prec, 256> getPrecedenceArray() {
    std::array<Prec, 256> precedence = {};
    
    precedence[static_cast<uint8_t>(Type::UNION)] = Prec::LOW;
    precedence[static_cast<uint8_t>(Type::CONCAT)] = Prec::MEDIUM;
    precedence[static_cast<uint8_t>(Type::STAR)] = Prec::HIGH;
    precedence[static_cast<uint8_t>(Type::QUESTION)] = Prec::HIGH;
    precedence[static_cast<uint8_t>(Type::PLUS)] = Prec::HIGH;
    precedence['('] = precedence[')'] = Prec::PARENTHESES;

    return precedence;
}

constexpr auto PRECEDENCE = getPrecedenceArray();

// function declarations

bool canStart(uint8_t);
bool canEnd(uint8_t);
std::vector<Token> regexToPostfix(const std::string&);
bool simulateNfa(State*, const std::string&);

