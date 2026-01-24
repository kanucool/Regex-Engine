#pragma once

#include <vector>
#include <string>
#include <array>
#include <stack>
#include <iostream>
#include <memory>
#include <unordered_set>

// constants

constexpr int NFA_ARENA_SIZE = 25;

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
    uint64_t compressionID = ULLONG_MAX;
    uint8_t c;

    State() = default;

    State(NodeType type, uint8_t c = 0) : type(type), c(c) {}
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

    State* makeState(NodeType type, uint8_t c = 0) {

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

