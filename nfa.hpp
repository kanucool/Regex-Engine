#pragma once

#include <vector>
#include <string>
#include <array>
#include <stack>
#include <iostream>
#include <memory>
#include <unordered_set>

// constants

using char_t = uint8_t;
using string = std::string;

constexpr int NFA_ARENA_SIZE = 64;

// data structures

enum class Type : char_t {
    LITERAL,
    CONCAT,
    STAR = '*',
    UNION = '|',
    DOT = '.',
    QUESTION = '?',
    PLUS = '+'
};

enum class Prec : char_t {
    LITERAL,
    SPECIAL,
    PARENTHESES,
    LOW,
    MEDIUM,
    HIGH
};

struct Token {
    Type type;
    char_t c;
};

enum class NodeType : char_t {
    LITERAL,
    SPLIT,
    MATCH,
    WILDCARD
};

struct State {
    State* out[2] = {nullptr, nullptr};
    NodeType type;
    char_t c;

    State() = default;

    State(NodeType type, char_t c = 0) : type(type), c(c) {}
};

struct Fragment {
    State* entry;
    std::vector<State**> exits;
};

class NFA {
private:
    int arenaIdx = NFA_ARENA_SIZE;
    std::vector<std::unique_ptr<State[]>> stateArenas;

public:
    State* start = nullptr;

    State* makeState(NodeType type, char_t c = 0) {

        if (arenaIdx >= NFA_ARENA_SIZE) {
            auto uPtr = std::make_unique<State[]>(NFA_ARENA_SIZE);
            stateArenas.push_back(std::move(uPtr));
            arenaIdx = 0;
        }
        
        stateArenas.back()[arenaIdx] = State(type, c);
        return &stateArenas.back()[arenaIdx++];
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
    
    precedence[static_cast<char_t>(Type::UNION)] = Prec::LOW;
    precedence[static_cast<char_t>(Type::CONCAT)] = Prec::MEDIUM;
    precedence[static_cast<char_t>(Type::STAR)] = Prec::HIGH;
    precedence[static_cast<char_t>(Type::QUESTION)] = Prec::HIGH;
    precedence[static_cast<char_t>(Type::PLUS)] = Prec::HIGH;
    precedence['('] = precedence[')'] = Prec::PARENTHESES;
    precedence['\\'] = precedence['^'] = Prec::SPECIAL;
    precedence['$'] = Prec::SPECIAL;

    return precedence;
}

constexpr auto PRECEDENCE = getPrecedenceArray();

// function declarations

bool canStart(char_t);
bool canEnd(char_t);
std::vector<Token> regexToPostfix(const string&);
bool simulateNfa(State*, const string&);

