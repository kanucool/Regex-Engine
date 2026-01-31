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

using char_t = char32_t;
//using string = std::string;

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

struct UTF8View : public std::string_view {
    using std::string_view::string_view;

    UTF8View(const std::string& s) : std::string_view(s) {}

    UTF8View(std::string&&) = delete;

    struct Iterator {
        const char* ptr;

        inline char8_t getNumBytes(char8_t c) const {
            if (!(c & 0x80)) return 1;
            if (!(c & 0x20)) return 2;
            if (!(c & 0x10)) return 3;
            if ((c & 0xF0) == 0xF0) return 4;
            // invalid utf8, fallback to 1byte
            return 1;
        }

        inline char_t getChar() const {
            uint8_t numBytes = getNumBytes(*ptr);

            // one byte
            char_t c = *ptr;
            if (numBytes == 1) return c;

            // two bytes
            char_t c2 = ptr[1] & 0x3F;
            if (numBytes == 2) return (c & 0x1F) << 6 | c2;
            
            // three bytes
            char_t c3 = ptr[2] & 0x3F;
            if (numBytes == 3) return (c & 0x0F) << 12 | c2 << 6 | c3;
            
            // four bytes
            char_t c4 = ptr[3] & 0x3F;
            return (c & 0x07) << 18 | c2 << 12 | c3 << 6 | c4;
        }

        char_t operator*() const {
            return getChar();
        }

        Iterator& operator++() {
            ptr += getNumBytes(*ptr);
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return ptr == other.ptr;
        }

        bool operator!=(const Iterator& other) const {
            return ptr != other.ptr;
        }     
    };

    Iterator begin() const {
        return Iterator{data()};
    }

    Iterator end() const {
        return Iterator{data() + size()};
    }

    void pop_back() {
        remove_suffix(1);
    }
};

// IMPORTANT
using string = UTF8View;

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
Prec getPrecedence(char_t);
bool escapedAtEnd(const string&); 
void mergeIntervals(std::vector<ClassInterval>&);
std::vector<Token> regexToPostfix(string);
bool simulateNfa(State*, const string&);
std::vector<char32_t> convertToUtf32(const std::string&);

