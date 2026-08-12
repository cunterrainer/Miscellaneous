#include "stack.h"

int main()
{
    stack<int> values;
    values.pop();
    return static_cast<int>(values.size());
}
