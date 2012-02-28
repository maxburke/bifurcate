#include <math.h>
#include "MathTypes.h"
#include "Core.h"

#include "emmintrin.h"

namespace bg
{
    void SoaQuatPos::Initialize(int numElements, void *memory)
    {
        const size_t elements = static_cast<size_t>(numElements);
        mNumElements = elements;
        const size_t strideElements = bc::RoundUp(size_t(SIMD_SIZE), elements);
        
        float * const x = static_cast<float *>(memory);
        this->mBase = x;
        this->mX = x;
        this->mY = x + strideElements;
        this->mZ = x + (2 * strideElements);
        this->mQx = x + (3 * strideElements);
        this->mQy = x + (4 * strideElements);
        this->mQz = x + (5 * strideElements);
        this->mQw = x + (6 * strideElements);
    }

    size_t SoaQuatPos::MemorySize(int numElements)
    {
        const size_t elements = static_cast<size_t>(numElements);
        const size_t strideElements = bc::RoundUp(size_t(SIMD_SIZE), elements);
        const size_t strideBytes = strideElements * sizeof(float);
        const size_t numRows = 7; // x, y, z, qx, qy, qz, qw
        const size_t allocationSize = strideBytes * numRows;
        return allocationSize;
    }

    void SoaQuatPos::UncompressQw()
    {
        ConversionUnion conversionUnion;
        conversionUnion.mUnsigned = 0x7ffffffful;

        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 absoluteValueMask = _mm_load1_ps(&conversionUnion.mFloat);

        float *qx = this->mQx;
        float *qy = this->mQy;
        float *qz = this->mQz;
        float *qw = this->mQw;

        for (size_t i = 0, e = (this->mNumElements / SIMD_SIZE); i != e; i += SIMD_SIZE)
        {
            const __m128 x = _mm_load_ps(qx + i);
            const __m128 y = _mm_load_ps(qy + i);
            const __m128 z = _mm_load_ps(qz + i);
            const __m128 x2 = _mm_mul_ps(x, x);
            const __m128 y2 = _mm_mul_ps(y, y);
            const __m128 z2 = _mm_mul_ps(z, z);
            const __m128 sum = _mm_add_ps(z2, _mm_add_ps(x2, y2));
            const __m128 oneMinus = _mm_sub_ps(one, sum);
            const __m128 abs = _mm_and_ps(oneMinus, absoluteValueMask);
            const __m128 w = _mm_sqrt_ps(abs);
            _mm_store_ps(qw + i, w);
        }
    }

    Quaternion UncompressQuaternion(const CompressedQuaternion &cq)
    {
        return Quaternion(cq.x, cq.y, cq.z, sqrt(fabs(1.0f - (cq.x * cq.x + cq.y * cq.y + cq.z * cq.z))));
    }
}
