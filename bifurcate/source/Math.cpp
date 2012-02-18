#include <math.h>
#include "MathTypes.h"

namespace bg
{
    quaternion uncompress_quaternion(const compressed_quaternion &cq)
    {
        return quaternion(cq.x, cq.y, cq.z, sqrt(fabs(1.0f - (cq.x * cq.x + cq.y * cq.y + cq.z * cq.z))));
    }
}
