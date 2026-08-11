#include <stdio.h>
#include "../../cstring.h"

int main(void)
{
    string value;
    string_create(&value, "abcXdef");
    printf("false find: %zu\n", string_find(&value, "aXc", 0));

    char output[8] = {0};
    printf("copied: %zu\n", string_copy(&value, output, 2, 3));
    printf("text: %s\n", output);
    string_free(&value);
    return 0;
}
