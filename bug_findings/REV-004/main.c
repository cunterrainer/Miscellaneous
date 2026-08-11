#include "../../cstring.h"

int main(void)
{
    string value;
    string_create_empty(&value);
    string_clear(&value);
    string_free(&value);
    return 0;
}
