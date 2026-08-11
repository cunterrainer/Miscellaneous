#include <iostream>
#include "../../stack_vector.h"

int main()
{
    stack_vector<int, 4> short_value{1};
    stack_vector<int, 4> long_value{1, 2};
    std::cout << (short_value == long_value) << '\n';
}
