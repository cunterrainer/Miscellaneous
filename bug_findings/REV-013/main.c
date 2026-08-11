#include <stdlib.h>
#include "../../Hash.h"

int main(void)
{
    char output[201];
    (void)hash_shake128_binary("x", 1, 200, output);
    return output[0] == '\0';
}
