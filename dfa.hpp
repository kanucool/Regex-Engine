#pragma once

#include "nfa.hpp"

#include <map>
#include <utility>
#include <algorithm>
#include <bitset>
#include <limits>
#include <concepts>

#include <unordered_dense.h>

// constants

constexpr int DFA_ARENA_SIZE = 1024;
constexpr int NFA_RESERVE = 4096;
constexpr char_t MAX_CHAR = std::numeric_limits<char_t>::max();

constexpr bool ADD = true;
constexpr bool REMOVE = false;

template <typename T>
concept Comparable = std::totally_ordered<T>;

template<typename K, typename V, typename Hash = ankerl::unordered_dense::hash<K>>
using HashMap = ankerl::unordered_dense::map<K, V, Hash>;

// templated functions

// Ankerl hash template - not my original work
template <>
struct ankerl::unordered_dense::hash<std::vector<State*>> {
    using is_avalanching = void;

    std::size_t operator()(std::vector<State*> const& v) const noexcept {
        std::size_t h = 0;
        for (State* p : v) {
            h = ankerl::unordered_dense::detail::wyhash::mix(h,
                reinterpret_cast<uintptr_t>(p)
            );
        }
        return h;
    }
};

// data structures

template <typename T>
struct Interval {
    char_t l, r;
    T item;

    auto operator<=>(const Interval&) const = default;
};

template <typename T>
struct SparseSet {
    HashMap<T, uint64_t> itemToIdx;
    std::vector<T> items;
    uint64_t N = 0;

    inline void insert(const T& item) {
        itemToIdx[item] = N++;
        items.push_back(item);
    }

    inline void remove(const T& item) {
        uint64_t idx = itemToIdx[item];

        if (idx < --N) {
            T& last = items.back();
            itemToIdx[last] = idx;
            std::swap(items[idx], last);
        }
        
        itemToIdx.erase(item);
        items.pop_back();
    }

    void clear() {
        itemToIdx.clear();
        items.clear();
        N = 0;
    }
};

struct DfaState {
    std::vector<Interval<DfaState*>> neighbors;
    std::vector<State*> nfaStates;
    bool isMatch = false;
    bool processed = false;
};

class DFA {
private:
    bool lazy = false;

    int arenaIdx = DFA_ARENA_SIZE;
    std::vector<std::unique_ptr<DfaState[]>> stateArenas;
    std::vector<Interval<State*>> stateRanges;
    std::vector<Interval<std::vector<State*>>> stateSetRanges;
    std::stack<DfaState*, std::vector<DfaState*>> stateStk;

    HashMap<std::vector<State*>, DfaState*> nfaSetMap;

    // for expandAndClean
    std::unordered_set<State*> nfaVisited;
    std::vector<State*> newStates;
    std::stack<State*, std::vector<State*>> splits;

    // for reconcile
    std::vector<Interval<std::vector<uint64_t>>> idxRes;
    SparseSet<uint64_t> itemSet;
    std::vector<uint64_t> freqs; 
      
    struct SweepItem {
        uint64_t point;
        uint64_t idx;
        bool type;

        auto operator<=>(const SweepItem&) const = default;
    };

    std::vector<SweepItem> line;

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

        stateRanges.clear();
        stateSetRanges.clear();

        idxRes.clear();
        itemSet.clear();
        freqs.clear();
    }
    
    template <Comparable T>
    inline void clean(std::vector<T>& vec) {
        std::sort(vec.begin(), vec.end());
        auto last = std::unique(vec.begin(), vec.end());
        vec.erase(last, vec.end());
    }

    template <typename T>
    void reconcile(
        std::vector<Interval<T>>& intervals,
        std::vector<Interval<std::vector<T>>>& dest
    ) {
        dest.clear();
        if (intervals.empty()) return;

        line.clear();
        uint64_t N = 0;

        for (auto& [l, r, item] : intervals) {
            line.push_back({l, N, ADD});
            line.push_back({static_cast<uint64_t>(r) + 1,
                            N++, REMOVE});
        }
        
        if (line.size() > 2) {
            std::sort(line.begin(), line.end());
        }

        idxRes.clear();
        idxRes.reserve(N << 1);
        dest.reserve(N << 1);
        freqs.resize(N);

        uint64_t lastPoint = line[0].point;
        
        for (auto [point, idx, type] : line) {
            if (itemSet.N && point > lastPoint) {
                idxRes.push_back({
                    static_cast<char_t>(lastPoint),
                    static_cast<char_t>(point - 1),
                    itemSet.items
                });
                lastPoint = point;
            }
            
            if (type == ADD) {
                if (!freqs[idx]++) itemSet.insert(idx);
            }
            else if (!--freqs[idx]) itemSet.remove(idx);
        } 
        
        for (auto& [l, r, vec] : idxRes) {
            std::vector<T> mapped;
            mapped.reserve(vec.size());

            std::transform(
                vec.begin(), vec.end(), std::back_inserter(mapped),
                [&intervals](uint64_t idx) {
                    return intervals[idx].item;
                }
            );

            dest.push_back({l, r, std::move(mapped)});
        }
    } 

    DfaState* findNeighbor(DfaState* curr, char_t c) {
        auto& neighbors = curr->neighbors;
        if (!neighbors.size()) return nullptr;

        auto res = std::lower_bound(
            neighbors.begin(),
            neighbors.end(),
            static_cast<uint64_t>(c),
            [](const auto& obj, uint64_t c) {
                return obj.l < c;
            }
        );

        if (res == neighbors.end() || res->l > c) {
            if (res == neighbors.begin()) return nullptr;
            --res;
        }
        return res->r < c ? nullptr : res->item;
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

    bool eval(const string& candidate);

    DFA() = default;

    DFA(const NFA& nfa, bool lazy = false) : lazy(lazy) {
        makeDfa(nfa.start);
    }
};

