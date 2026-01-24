#include "dfa.hpp"

void DFA::expandAndClean(std::vector<State*>& nfaStates) {

    nfaVisited.clear();
    newStates.clear();

    std::unordered_set<State*>& lVisited = nfaVisited;
    std::stack<State*>& lSplits = splits;

    auto push = [&lVisited, &lSplits](State* state) {
        if (lVisited.find(state) == lVisited.end()) {
            lVisited.insert(state);
            lSplits.push(state);
        }
    };

    for (State* state : nfaStates) {
        if (state->type != NodeType::SPLIT) {
            newStates.push_back(state);
        }
        else push(state);
    }

    while (!splits.empty()) {
        State* state = splits.top();
        splits.pop();
        
        if (state->type != NodeType::SPLIT) {
            newStates.push_back(state);
        }
        else {
            push(state->out[0]);
            push(state->out[1]);
        }
    }

    nfaStates = std::move(newStates);
    cleanSet(nfaStates);
}

DfaState* DFA::makeDfa(State* startState) {
    
    clearDfa();
    if (!startState) return nullptr;
    nfaSetMap.reserve(NFA_RESERVE);

    std::vector<State*> buckets[256];
 
    DfaState* ans = createEmptyState();
    this->start = ans;

    std::vector<State*> startStates;
    startStates.push_back(startState);

    expandAndClean(startStates);
    nfaSetMap[startStates] = ans;

    std::stack<std::pair<DfaState*, std::vector<State*>>> stk;
    stk.push({ans, std::move(startStates)});

    while (!stk.empty()) {
        DfaState* newState = stk.top().first;
        std::vector<State*> nfaStates = std::move(stk.top().second);
        stk.pop();

        newState->nfaStates = std::move(nfaStates);

        for (int i = 0; i < 256; i++) {
            buckets[i].clear();
        }
        
        for (State* nfaState : newState->nfaStates) {
            if (nfaState->type == NodeType::LITERAL) {
                buckets[nfaState->c].push_back(nfaState->out[0]);
            }
            else if (nfaState->type == NodeType::WILDCARD) {
                State* out = nfaState->out[0];

                for (int i = 0; i < 256; i++) {
                    buckets[i].push_back(out);
                }
            }
            else if (nfaState->type == NodeType::MATCH) {
                newState->isMatch = true;
            }
        }

        for (int i = 0; i < 256; i++) {
            if (buckets[i].empty()) continue;

            expandAndClean(buckets[i]);

            newState->neighbors[i] = nfaSetMap[buckets[i]];
            
            if (!newState->neighbors[i]) {
                DfaState* neighborState = createEmptyState();

                nfaSetMap[buckets[i]] = neighborState;
                newState->neighbors[i] = neighborState;

                stk.push({neighborState, std::move(buckets[i])});
            }
        }
    }

    return ans;
}

bool simulateDfa(DfaState* curr, const std::string& candidate) {
    if (!curr) return candidate.empty();

    for (uint8_t c : candidate) {
        if (!curr->neighbors[c]) return false;
        curr = curr->neighbors[c];
    }

    return curr->isMatch;
}

