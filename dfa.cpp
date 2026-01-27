#include "dfa.hpp"

void DFA::expandAndClean(std::vector<State*>& nfaStates) {

    nfaVisited.clear();
    newStates.clear();

    std::unordered_set<State*>& lVisited = nfaVisited;
    std::stack<State*, std::vector<State*>>& lSplits = splits;

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
    clean(nfaStates);
}

void DFA::fillNeighbors(DfaState* newState) {
    if (newState->processed) return;

    stateRanges.clear();
    
    for (State* nfaState : newState->nfaStates) {
        State* out = nfaState->out[0];

        if (nfaState->type == NodeType::LITERAL) {
            char_t c = nfaState->c;
            stateRanges.push_back({c, c, out});
        }
        else if (nfaState->type == NodeType::WILDCARD) {
            stateRanges.push_back({0, MAX_CHAR, out});
        }
        else if (nfaState->type == NodeType::MATCH) {
            newState->isMatch = true;
        }
    }

    clean(stateRanges);
    reconcile(stateRanges, stateSetRanges);

    for (auto& [l, r, nfaStates] : stateSetRanges) {
        if (nfaStates.empty()) continue;
        expandAndClean(nfaStates);

        DfaState* neighborState = nfaSetMap[nfaStates];
        
        if (!neighborState) {
            neighborState = createEmptyState();
            nfaSetMap[nfaStates] = neighborState;
            neighborState->nfaStates = std::move(nfaStates);

            if (!lazy) stateStk.push(neighborState);
            nfaStates.clear();
        }

        newState->neighbors.push_back({l, r, neighborState});
    }

    newState->processed = true;
}

DfaState* DFA::makeDfa(State* startState) {
    clearDfa();
    if (!startState) return nullptr;
    nfaSetMap.reserve(NFA_RESERVE);
 
    DfaState* ans = createEmptyState();
    this->start = ans;

    std::vector<State*> startStates;
    startStates.push_back(startState);

    expandAndClean(startStates);
    nfaSetMap[startStates] = ans;
    ans->nfaStates = std::move(startStates);
    
    if (lazy) return ans;

    stateStk.push(ans);

    while (!stateStk.empty()) {
        DfaState* newState = stateStk.top();
        stateStk.pop();

        fillNeighbors(newState);
        newState->nfaStates.clear();
    }

    return ans;
}

bool DFA::eval(const string& candidate) {
    DfaState* curr = start;
    if (curr == nullptr) return candidate.empty();

    for (char_t c : candidate) {
        if (!curr->processed) fillNeighbors(curr);
        curr = findNeighbor(curr, c);
        if (curr == nullptr) return false;
    }
    if (!curr->processed) fillNeighbors(curr);
    return curr->isMatch;
}

