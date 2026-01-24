#pragma once

#include "nfa.hpp"

#include <map>
#include <utility>
#include <algorithm>
#include <bitset>

// constants

constexpr int DFA_ARENA_SIZE = 256;
constexpr int COMPRESSED_LIMIT = 64;

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

struct PtrVecEquality {
    template <typename T>
    bool operator()(const std::vector<T*>& a, const std::vector<T*>& b) const {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }
};

struct NfaSet {
    // for <= COMPRESSED_LIMIT nodes
    std::bitset<COMPRESSED_LIMIT> mask = 0;
    // for > COMPRESSED_LIMIT nodes
    std::vector<State*> nfaStates;
};

struct DfaState {
    DfaState* neighbors[256] = {nullptr};
    NfaSet nfaSet;
    bool isMatch = false;
};

class DFA {
private:
    int arenaIdx = DFA_ARENA_SIZE;
    std::vector<std::unique_ptr<DfaState[]>> stateArenas;

    std::unordered_map<std::vector<State*>, DfaState*,
                       PtrVecHash, PtrVecEquality> nfaSetMap;

    // use compression for the first COMPRESSED_LIMIT NFA states
    bool exceededCompression = false;
    uint64_t numRegistered = 0;
    State* compressedNfaArr[COMPRESSED_LIMIT] = {nullptr};
    std::unordered_map<std::bitset<COMPRESSED_LIMIT>, DfaState*> compressedNfaSetMap;

    // for expandAndClean
    std::unordered_set<State*> nfaVisited;
    NfaSet newSet;
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
        newSet = NfaSet(); 

        for (uint64_t i = 0; i < numRegistered; i++) {
            if (compressedNfaArr[i]) {
                compressedNfaArr[i]->compressionID = ULLONG_MAX;
                compressedNfaArr[i] = nullptr;
            }
        }

        numRegistered = 0;
        exceededCompression = false;
        compressedNfaSetMap.clear();
    }

    inline void migrateToUncompressed() {
        if (!exceededCompression) return;

        for (const auto [mask, state] : compressedNfaSetMap) {
            NfaSet nfaSet;

            for (uint64_t i = 0; i < COMPRESSED_LIMIT; i++) {
                if (mask.test(i)) {
                    nfaSet.nfaStates.push_back(compressedNfaArr[i]);
                }
            }

            cleanSet(nfaSet);
            nfaSetMap[std::move(nfaSet.nfaStates)] = state;
        }
    }

    inline void cleanSet(NfaSet& nfaSet) {
        if (!exceededCompression) return;

        std::vector<State*>& nfaStates = nfaSet.nfaStates;
        std::sort(nfaStates.begin(), nfaStates.end());
        auto last = std::unique(nfaStates.begin(), nfaStates.end());
        nfaStates.erase(last, nfaStates.end());
    }

    void clearSet(NfaSet& nfaSet) {
        if (!exceededCompression) nfaSet.mask.reset();
        nfaSet.nfaStates.clear();
    }

    void addNfaStateToSet(NfaSet& nfaSet, State* state) {
        nfaSet.nfaStates.push_back(state);
        if (!exceededCompression) {
            nfaSet.mask.set(state->compressionID);
        }
    }

    void registerNfaState(State* state) {
        if (exceededCompression) return;

        if (state->compressionID == ULLONG_MAX) {
            if (numRegistered == COMPRESSED_LIMIT) {
                exceededCompression = true;
                migrateToUncompressed();
            }
            else {
                compressedNfaArr[numRegistered] = state;
                state->compressionID = numRegistered++;
            }
        }
    }

    DfaState* getDfaFromNfaSet(NfaSet& nfaSet) {
        if (exceededCompression) {
            std::vector<State*> nfaStates = nfaSet.nfaStates;

            if (auto it = nfaSetMap.find(nfaStates);
                                it  != nfaSetMap.end()) {
                return it->second;
            }
        }
        else if (auto it = compressedNfaSetMap.find(nfaSet.mask);
                                it != compressedNfaSetMap.end()) {
            return it->second;
        }
        
        return nullptr;
    }

    void insertNfaSet(NfaSet& nfaSet, DfaState* dfaState) {
        if (exceededCompression) {
            nfaSetMap[nfaSet.nfaStates] = dfaState;
        }
        else {
            compressedNfaSetMap[nfaSet.mask] = dfaState;
        }
    }

    DfaState* createEmptyState() {

        if (arenaIdx >= DFA_ARENA_SIZE) {
            auto uPtr = std::make_unique<DfaState[]>(DFA_ARENA_SIZE);
            stateArenas.push_back(std::move(uPtr));
            arenaIdx = 0;
        }
        
        return &stateArenas.back()[arenaIdx++];
    }
    
    void expandAndClean(NfaSet& nfaSet);
    DfaState* makeDfa(State* startState);

    DFA() = default;

    DFA(const NFA& nfa) {
        makeDfa(nfa.start);
    }
};

// function declarations

bool simulateDfa(DfaState*, const std::string&);

