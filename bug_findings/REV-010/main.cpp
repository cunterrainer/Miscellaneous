#include "../../Vector.h"

int main()
{
    const Vector3<float> a(1.0f, 2.0f, 3.0f);
    const Vector3<float> b(4.0f, 5.0f, 6.0f);
    auto projected = a.ProjectOnto(b);
    (void)projected;
}
