#include <utility>
#include "../../array.h"
#include "../../stack.h"

int main()
{
    (void)get<0>(array<int, 1>{1});
    const stack<int> left;
    const stack<int> right;
    swap(left, right);
}
