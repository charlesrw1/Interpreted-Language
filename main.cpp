#include "lang.h"
#include <iostream>
#include <fstream>


int main()
{
    Interpreter::init_macros();
    Interpreter inter;

    inter.init_variable("welcome", Variable::STR);
    inter.set_variable("welcome", "Hello from C++");

    std::ifstream infile("examples/global_vars.txt");
    if (!infile)
        return 1;
    std::string line;
    inter.interpret(infile);
    std::cout << *Interpreter::get_variable("welcome").str << std::endl;

    std::cout << ">> ";

    while (std::getline(std::cin, line))
    {
        inter.interpret(line);
        std::cout << ">> ";
    }
    std::cin.get();
    return 0;
};