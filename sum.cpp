// use -Wa,-mbig-obj to compile
#include <iostream>

#include "exprtk.hpp"

void print_help(const char* path)
{
    std::cout << "Usage: " << path << " [start (optional)] [end] [expression]" << std::endl;
    std::cout << "       Used iterator variable is i" <<std::endl;
    std::cout << "       Supported expressions:" << std::endl;
    std::cout << "       +, -, *, /, %, ^" << std::endl;
    std::cout << "       min, max, avg, sum, abs, ceil, floor, round, roundn, exp, log, log10, logn, pow, root, sqrt, clamp, inrange, swap" << std::endl;
    std::cout << "       sin, cos, tan, acos, asin, atan, atan2, cosh, cot, csc, sec, sinh, tanh, deg2rad, rad2deg, deg2grad, grad2deg, hypot" << std::endl;
    std::cout << "       =, ==, <>, !=, <, <=, >, >=" << std::endl;
    std::cout << "       :=, +=, -=, *=, /=, %=" << std::endl;
    std::cout << "       and, nand, nor, not, or, xor, xnor, mand, mor" << std::endl;
    std::cout << "       if-then-else, ternary conditional, switch case, return-statement" << std::endl;
    std::cout << "       while loop, for loop, repeat until loop, break, continue" << std::endl;
}

int main(int argc, const char** argv)
{
    int start = 0;
    int end = 0;

    for (int i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
    }

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " [start (optional)] [end (optional)] [expression]\nTry '--help' for additional help" << std::endl;
        return 1;
    }

    try
    {
        if (argc == 3)
        {
            end = std::stoi(argv[1]);
        }
        else if (argc == 4)
        {
            start = std::stoi(argv[1]);
            end = std::stoi(argv[2]);
        }
        else if (argc > 4)
        {
            std::cerr << "Too many arguments\n" << std::endl;
            return 0;
        }

        if (end < start)
        {
            std::cerr << "Start (" << start << ") has to be less than end (" << end << ")" << std::endl;
            return 1;
        }
    }
    catch (const std::invalid_argument& ex)
    {
        std::cerr << "Failed to convert argument to int: " << ex.what() << std::endl;
        return 1;
    }

    
    double i = start;
    double result = 0;
    for (; i <= end; ++i)
    {
        exprtk::symbol_table<double> symbol_table;
        symbol_table.add_variable("i", i);
        symbol_table.add_constants();

        exprtk::expression<double> expression;
        expression.register_symbol_table(symbol_table);

        exprtk::parser<double> parser;
        parser.compile(argv[argc - 1], expression);
        result += expression.value();
    }
    std::cout << "Result: " << result << std::endl;
}