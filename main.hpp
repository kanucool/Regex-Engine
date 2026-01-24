#include "dfa.hpp"

#include <chrono>

// constants

const char* message = R"(1. Eval DFA
2. Eval NFA
3. Set Regex
4. Exit
Choice: )";

const char* dfaOrNfa = R"(DFA or NFA?
(Note that DFA construction may take time while NFA construction is very quick).
(DFA / NFA): )";

const char* bools[2] = {"false", "true"};

// data structures

class Regex {
private:
    DFA dfa;
    NFA nfa;
    std::string regex;

public:
    Regex(const std::string& regex, bool makeDfa = false) {
        this->regex = regex;
        nfa = NFA(regexToPostfix(regex));
        if (makeDfa) dfa = DFA(nfa);
    }

    Regex() = default;

    void setRegex(const std::string& regex, bool makeDfa = true) {
        this->regex = regex;
        nfa = NFA(regexToPostfix(regex));
        if (makeDfa) dfa = DFA(nfa);
    }

    bool eval(const std::string& candidate) {
        if (dfa.start == nullptr) {
            return evalNfa(candidate);
        }
        return evalDfa(candidate);
    }

    bool evalDfa(const std::string& candidate) {
        return simulateDfa(dfa.start, candidate);
    }

    bool evalNfa(const std::string& candidate) {
        return simulateNfa(nfa.start, candidate);
    }
};

