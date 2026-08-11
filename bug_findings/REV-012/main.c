#include "../../Hash.h"

int main(void)
{
    return hash_md5_easy("review")[0] == '\0';
}
