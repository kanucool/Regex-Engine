#pragma once

#include "nfa.hpp"

#include <map>
#include <utility>
#include <algorithm>

// data structures

struct DfaState {
    DfaState* neighbors[256] = {nullptr};
    std::vector<State*> NfaStates;
    bool isMatch = false;
};

class DFA {
private:
    std::vector<std::unique_ptr<DfaState>> states;
    std::map<std::vector<State*>, DfaState*> nfaSetMap;

    std::unordered_set<State*> nfaVisited;
    std::vector<State*> newStates;
    std::stack<State*> splits;

public:
    DfaState* start;

    DfaState* createEmptyState() {
        auto uPtr = std::make_unique<DfaState>();
        DfaState* newState = uPtr.get();
        states.push_back(std::move(uPtr));
        return newState;
    }

    void expandAndClean(std::vector<State*>& nfaStates);
    DfaState* makeDfa(std::vector<State*> startStates);

    DFA(const NFA& nfa) {
        makeDfa({nfa.start});
    }
};

// function declarations

bool simulateDfa(DfaState*, const std::string&);

