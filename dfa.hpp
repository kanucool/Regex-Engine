#pragma once

#include "nfa.hpp"

#include <map>
#include <utility>
#include <algorithm>
#include <bitset>

#include <unordered_dense.h>

// constants

constexpr int DFA_ARENA_SIZE = 1024;
constexpr int NFA_RESERVE = 4096;

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
    bool processed = false;
};

class DFA {
private:
    bool lazy = false;

    int arenaIdx = DFA_ARENA_SIZE;
    std::vector<std::unique_ptr<DfaState[]>> stateArenas;
    std::vector<State*> buckets[256];
    std::stack<DfaState*, std::vector<DfaState*>> stateStk;


    ankerl::unordered_dense::map<std::vector<State*>, DfaState*,
                                 PtrVecHash,
                                 std::equal_to<std::vector<State*>>> nfaSetMap;

    //std::unordered_map<std::vector<State*>, DfaState*,
                       //PtrVecHash, PtrVecEquality> nfaSetMap;

    // for expandAndClean
    std::unordered_set<State*> nfaVisited;
    std::vector<State*> newStates;
    std::stack<State*, std::vector<State*>> splits;

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

        for (int i = 0; i < 256; i++) {
            buckets[i].clear();
        }
    }

    inline void cleanSet(std::vector<State*>& nfaStates) {
        std::sort(nfaStates.begin(), nfaStates.end());
        auto last = std::unique(nfaStates.begin(), nfaStates.end());
        nfaStates.erase(last, nfaStates.end());
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
    void fillNeighbors(DfaState* newState);
    DfaState* makeDfa(State* startState);

    bool eval(const std::string& candidate);

    DFA() = default;

    DFA(const NFA& nfa, bool lazy = false) : lazy(lazy) {
        makeDfa(nfa.start);
    }
};

