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

    std::swap(nfaStates, newStates);
    std::sort(nfaStates.begin(), nfaStates.end());
    auto last = std::unique(nfaStates.begin(), nfaStates.end());
    nfaStates.erase(last, nfaStates.end());
}

DfaState* DFA::makeDfa(std::vector<State*> startStates) {

    std::vector<State*> buckets[256];
 
    DfaState* ans = createEmptyState();
    this->start = ans;
    
    expandAndClean(startStates);
    nfaSetMap[startStates] = ans;

    std::stack<std::pair<DfaState*, std::vector<State*>>> stk;
    stk.push({ans, std::move(startStates)});

    while (!stk.empty()) {
        DfaState* newState = stk.top().first;
        auto nfaStates = std::move(stk.top().second);
        stk.pop();

        newState->NfaStates = nfaStates;

        for (int i = 0; i < 256; i++) {
            buckets[i].clear();
        }
        
        for (State* nfaState : newState->NfaStates) {
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
            expandAndClean(buckets[i]);

            if (nfaSetMap.find(buckets[i]) != nfaSetMap.end()) {
                newState->neighbors[i] = nfaSetMap[buckets[i]];
            }
            else {
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
    if (!curr) return false;

    for (uint8_t c : candidate) {
        if (!curr->neighbors[c]) return false;
        curr = curr->neighbors[c];
    }

    return curr->isMatch;
}

int main() {
    std::cout << "regex: ";
    std::string regex; std::cin >> regex;

    DFA dfa(NFA(regexToPostfix(regex)));
    
    while (true) {
        std::cout << "candidate: ";
        std::string candidate; std::cin >> candidate;
        std::cout << simulateDfa(dfa.start, candidate) << std::endl;
    }

    return 0;
}
