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
    Regex(const std::string& regex, bool makeDfa = false, bool lazy = false) {
        this->regex = regex;
        nfa = NFA(regexToPostfix(std::move(regex)));
        if (makeDfa) dfa = DFA(nfa, lazy);
    }

    Regex() = default;

    DFA& getDfa() {return dfa;}
    NFA& getNfa() {return nfa;}

    void setRegex(const std::string& regex, bool makeDfa = false, bool lazy = false) {
        this->regex = regex;
        nfa = NFA(regexToPostfix(std::move(regex)));
        if (makeDfa) dfa = DFA(nfa, lazy);
    }

    bool eval(const std::string& candidate) {
        if (dfa.start == nullptr) {
            return evalNfa(candidate);
        }
        return evalDfa(candidate);
    }

    bool evalDfa(const std::string& candidate) {
        return dfa.eval(candidate);
    }

    bool evalNfa(const std::string& candidate) {
        return simulateNfa(nfa.start, candidate);
    }
};

