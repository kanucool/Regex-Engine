#pragma once

#include "nfa.hpp"

#include <map>
#include <utility>
#include <algorithm>
#include <bitset>

#include<unordered_dense.h>

// constants

constexpr int DFA_ARENA_SIZE = 512;
constexpr int NFA_RESERVE = 2048;

// data structures

// // Fowler-Noll-Vo style hash template
struct PtrVecHash {
    template <typename T>
    std::size_t operator()(const std::vector<T*>& vec) const {
        std::size_t seed = vec.size();
        for (auto ptr : vec) {
            seed ^= std::hash<T*>{}(ptr) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

struct DfaState {
    DfaState* neighbors[256] = {nullptr};
    std::vector<State*> nfaStates;
    bool isMatch = false;
};

class DFA {
private:
    int arenaIdx = DFA_ARENA_SIZE;
    std::vector<std::unique_ptr<DfaState[]>> stateArenas;

    ankerl::unordered_dense::map<std::vector<State*>, DfaState*,
                                 PtrVecHash,
                                 std::equal_to<std::vector<State*>>> nfaSetMap;

    //std::unordered_map<std::vector<State*>, DfaState*,
                       //PtrVecHash, PtrVecEquality> nfaSetMap;

    // for expandAndClean
    std::unordered_set<State*> nfaVisited;
    std::vector<State*> newStates;
    std::stack<State*> splits;

public:
    DfaState* start = nullptr;

    inline int numStates() {
        return DFA_ARENA_SIZE * (stateArenas.size() - 1) + arenaIdx;
    }

    void clearDfa() {
        start = nullptr;
        stateArenas.clear();
        arenaIdx = DFA_ARENA_SIZE;

        nfaSetMap.clear();
        nfaVisited.clear();
        newStates.clear();
    }

    inline void cleanSet(std::vector<State*>& nfaStates) {
        std::sort(nfaStates.begin(), nfaStates.end());
        auto last = std::unique(nfaStates.begin(), nfaStates.end());
        nfaStates.erase(last, nfaStates.end());
    }

    DfaState* getDfaFromNfaSet(std::vector<State*>& nfaStates) {
        if (auto it = nfaSetMap.find(nfaStates);
                          it != nfaSetMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void insertNfaSet(std::vector<State*>& nfaStates, DfaState* dfaState) {
        nfaSetMap[nfaStates] = dfaState;
    }

    DfaState* createEmptyState() {

        if (arenaIdx >= DFA_ARENA_SIZE) {
            auto uPtr = std::make_unique<DfaState[]>(DFA_ARENA_SIZE);
            stateArenas.push_back(std::move(uPtr));
            arenaIdx = 0;
        }
        
        return &stateArenas.back()[arenaIdx++];
    }
    
    void expandAndClean(std::vector<State*>& nfaStates);
    DfaState* makeDfa(State* startState);

    DFA() = default;

    DFA(const NFA& nfa) {
        makeDfa(nfa.start);
    }
};

// function declarations

bool simulateDfa(DfaState*, const std::string&);

