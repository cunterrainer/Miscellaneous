#include "stack.h"

int main()
{
    stack<int> values;
    volatile int value = values.top();
    return value;
}
