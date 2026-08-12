#include "array.h"

int main()
{
    array<int, 0> values{};
    volatile int value = values[0];
    return value;
}
