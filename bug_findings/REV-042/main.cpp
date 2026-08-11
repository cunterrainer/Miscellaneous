#include "../../zip_file.h"

int main()
{
    miniz_cpp::zip_file archive;
    return archive.namelist().empty() ? 0 : 1;
}
