#pragma once

#include <vector>
#include <string>
#include <array>
#include <stack>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_set>

// constants

using char_t = char8_t;
using string = std::string;

constexpr int NFA_ARENA_SIZE = 256;

// data structures

enum class Type : char_t {
    LITERAL,
    CONCAT,
    CLASS,
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
    SQUARE_BRACKETS,
    LOW,
    MEDIUM,
    HIGH
};

struct ClassInterval {
    char_t l, r;

    auto operator<=>(const ClassInterval&) const = default;
};

struct Token {
    Type type;
    char_t c;
    std::vector<ClassInterval> ranges;
};

enum class NodeType : char_t {
    LITERAL,
    SPLIT,
    RANGES,
    MATCH,
    WILDCARD
};

struct State {
    State* out[2] = {nullptr, nullptr};
    NodeType type;
    char_t c;
    std::vector<ClassInterval> ranges;

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

    State* makeState(NodeType type, char_t c = 0,
                     std::optional<std::vector<ClassInterval>> ranges = std::nullopt) {

        if (arenaIdx >= NFA_ARENA_SIZE) {
            auto uPtr = std::make_unique<State[]>(NFA_ARENA_SIZE);
            stateArenas.push_back(std::move(uPtr));
            arenaIdx = 0;
        }
        
        stateArenas.back()[arenaIdx] = State(type, c);
        if (ranges) {
            stateArenas.back()[arenaIdx].ranges = std::move(ranges.value());
        }
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
    precedence['['] = precedence[']'] = Prec::SQUARE_BRACKETS;
    precedence['\\'] = Prec::SPECIAL;

    return precedence;
}

constexpr auto PRECEDENCE = getPrecedenceArray();

// function declarations

bool canStart(char_t);
bool canEnd(char_t);
void mergeIntervals(std::vector<ClassInterval>&);
// search range
Prec getPrecedence(char_t);
std::vector<Token> regexToPostfix(const string&);
bool simulateNfa(State*, const string&);

