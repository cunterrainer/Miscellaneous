#include "array.h"

int main()
{
    array<int, 2> values{1, 2};
    volatile int index = 2;
    values[static_cast<array<int, 2>::size_type>(index)] = 3;
    return values[0];
}
