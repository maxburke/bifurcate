#include <math.h>
#include "MathTypes.h"

namespace bg
{
    Quaternion UncompressQuaternion(const CompressedQuaternion &cq)
    {
        return Quaternion(cq.x, cq.y, cq.z, sqrt(fabs(1.0f - (cq.x * cq.x + cq.y * cq.y + cq.z * cq.z))));
    }
}
