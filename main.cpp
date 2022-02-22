#include "lang.h"
#include <iostream>
#include <fstream>


int main()
{
    Interpreter::init_macros();
    Interpreter inter;
    std::ifstream infile("examples/fib.txt");
    if (!infile)
        return 1;
    std::string line;
    inter.interpret(infile);
    std::cout << ">>> ";

    while (std::getline(std::cin, line))
    {
        inter.interpret(line);
        std::cout << ">>> ";
    }
    std::cin.get();
    return 0;
};