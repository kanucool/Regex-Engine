#include "dfa.hpp"

void DFA::expandAndClean(NfaSet& nfaSet) {

    nfaVisited.clear();
    clearSet(newSet);

    std::unordered_set<State*>& lVisited = nfaVisited;
    std::stack<State*>& lSplits = splits;

    auto push = [&lVisited, &lSplits](State* state) {
        if (lVisited.find(state) == lVisited.end()) {
            lVisited.insert(state);
            lSplits.push(state);
        }
    };

    for (State* state : nfaSet.nfaStates) {
        if (state->type != NodeType::SPLIT) {
            registerNfaState(state);
            addNfaStateToSet(newSet, state);
        }
        else push(state);
    }

    while (!splits.empty()) {
        State* state = splits.top();
        splits.pop();
        
        if (state->type != NodeType::SPLIT) {
            registerNfaState(state);
            addNfaStateToSet(newSet, state);
        }
        else {
            push(state->out[0]);
            push(state->out[1]);
        }
    }

    nfaSet = std::move(newSet);
    cleanSet(nfaSet);
}

DfaState* DFA::makeDfa(State* startState) {
    
    clearDfa();
    if (!startState) return nullptr;

    NfaSet buckets[256];
 
    DfaState* ans = createEmptyState();
    this->start = ans;

    NfaSet startSet;
    registerNfaState(startState);
    addNfaStateToSet(startSet, startState);

    expandAndClean(startSet);
    insertNfaSet(startSet, ans);

    std::stack<std::pair<DfaState*, NfaSet>> stk;
    stk.push({ans, std::move(startSet)});

    while (!stk.empty()) {
        DfaState* newState = stk.top().first;
        NfaSet nfaSet = std::move(stk.top().second);
        stk.pop();

        newState->nfaSet = std::move(nfaSet);

        for (int i = 0; i < 256; i++) {
            clearSet(buckets[i]);
        }
        
        for (State* nfaState : newState->nfaSet.nfaStates) {
            if (nfaState->type == NodeType::LITERAL) {
                registerNfaState(nfaState->out[0]);
                addNfaStateToSet(buckets[nfaState->c],
                                    nfaState->out[0]);
            }
            else if (nfaState->type == NodeType::WILDCARD) {
                State* out = nfaState->out[0];
                registerNfaState(out);

                for (int i = 0; i < 256; i++) {
                    addNfaStateToSet(buckets[i], out);
                }
            }
            else if (nfaState->type == NodeType::MATCH) {
                newState->isMatch = true;
            }
        }

        for (int i = 0; i < 256; i++) {
            expandAndClean(buckets[i]);

            newState->neighbors[i] = getDfaFromNfaSet(buckets[i]);
            
            if (!newState->neighbors[i]) {
                DfaState* neighborState = createEmptyState();

                insertNfaSet(buckets[i], neighborState);
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

