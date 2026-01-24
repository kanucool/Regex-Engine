#include "main.hpp"

auto currTime = []() { return std::chrono::high_resolution_clock::now(); };

int main() {
    Regex evaluator;
    std::string regex, choice;

    std::chrono::high_resolution_clock::time_point start;

    while (true) {
        char response;
        std::cout << message;
        std::cin >> response;

        if (response == '1') {
            std::string candidate;
            std::cin >> candidate;

            start = currTime();
            std::cout << bools[evaluator.evalDfa(candidate)] << '\n';
        }
        else if (response == '2') {
            std::string candidate;
            std::cin >> candidate;

            start = currTime();
            std::cout << bools[evaluator.evalNfa(candidate)] << '\n';
        }
        else if (response == '3') {
            std::cout << "regex: ";
            std::cin >> regex;
            std::cout << dfaOrNfa;

            std::cin >> choice;
            
            start = currTime();

            if (choice == "DFA") {
                evaluator.setRegex(regex, true);
            }
            else {
                evaluator.setRegex(regex, false);
            }
        }
        else if (response == '4') break;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;

        std::cout << "------------------ " << diff.count() << " seconds\n";
    }

    return 0;
}

