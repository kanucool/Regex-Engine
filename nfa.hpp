#pragma once

#include <vector>
#include <string>
#include <array>
#include <stack>
#include <iostream>

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

