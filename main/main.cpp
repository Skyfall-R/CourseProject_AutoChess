#include <lib.hpp>

#include <iostream>

#include <string>

#include <cstdlib>



int main() {
    srand(time(nullptr));
    std::cout << "Welcome to Auto Chess\n1. Single Player vs AI\n2. Two Player\nInput mode: ";
    std::string mode;
    std::cin >> mode;
    bool vs_ai = mode != "2";
    Game g(vs_ai);
    g.start();
    return 0;
}

