#include <iostream>
#include "../../Result.h"

int main()
{
    Error<> error("number %d", 42);
    std::cout << "logical-size=" << error.what().size()
              << " text=" << error.what().c_str() << '\n';
}
