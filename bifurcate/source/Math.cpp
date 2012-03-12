#include <math.h>
#include <string.h>
#include "MathTypes.h"
#include "Core.h"

#include "emmintrin.h"

namespace bg
{
    void Mat4x4::SetIdentity()
    {
        memset(this, 0, sizeof *this);
        this->v[0] = 1;
        this->v[5] = 1;
        this->v[10] = 1;
        this->v[15] = 1;
    }

    void SoaQuatPos::Initialize(int numElements, void *memory)
    {
        this->mNumElements = numElements;
        const int strideElements = bc::RoundUp(SIMD_SIZE, numElements);
        
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
        const size_t strideBytes = strideElements * (int)sizeof(float);
        const size_t numRows = 7; // x, y, z, qx, qy, qz, qw
        const size_t allocationSize = strideBytes * numRows;
        return allocationSize;
    }

    static __forceinline __m128 Abs(__m128 a)
    {
        ConversionUnion conversionUnion;
        conversionUnion.mUnsigned = 0x7ffffffful;
        const __m128 absoluteValueMask = _mm_load1_ps(&conversionUnion.mFloat);
        return _mm_and_ps(a, absoluteValueMask);
    }

    void SoaQuatPos::UncompressQw()
    {
        const __m128 one = _mm_set1_ps(1.0f);

        float *qx = this->mQx;
        float *qy = this->mQy;
        float *qz = this->mQz;
        float *qw = this->mQw;

        for (int i = 0, e = this->mNumElements; i < e; i += SIMD_SIZE)
        {
            const __m128 x = _mm_load_ps(qx + i);
            const __m128 y = _mm_load_ps(qy + i);
            const __m128 z = _mm_load_ps(qz + i);
            const __m128 x2 = _mm_mul_ps(x, x);
            const __m128 y2 = _mm_mul_ps(y, y);
            const __m128 z2 = _mm_mul_ps(z, z);
            const __m128 sum = _mm_add_ps(z2, _mm_add_ps(x2, y2));
            const __m128 oneMinus = _mm_sub_ps(one, sum);
            const __m128 abs = Abs(oneMinus);
            const __m128 w = _mm_sqrt_ps(abs);
            _mm_store_ps(qw + i, w);
        }
    }

    static __forceinline __m128 _mm_madd_ps(__m128 a, __m128 b, __m128 c)
    {
        return _mm_add_ps(c, _mm_mul_ps(a, b));
    }

    static __forceinline __m128 _mm_sel_ps(__m128 f, __m128 t, __m128 mask)
    {
        return _mm_or_ps(_mm_and_ps(mask, t), _mm_andnot_ps(mask, f));
    }

    static __forceinline __m128 _mm_nmsub_ps(__m128 a, __m128 b, __m128 c)
    {
        return _mm_sub_ps(c, _mm_mul_ps(a, b));
    }
    
    static __forceinline __m128 Acos(__m128 a)
    {
        // Many thanks to Bullet Physics for this.
        const __m128 abs = Abs(a);
        const __m128 mask = _mm_cmplt_ps(a, _mm_setzero_ps());
        const __m128 t1 = _mm_sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.0f), abs));

        const __m128 abs2 = _mm_mul_ps(abs, abs);
        const __m128 abs4 = _mm_mul_ps(abs2, abs2);
        const __m128 hi = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(-0.0012624911f),
            abs, _mm_set1_ps(0.0066700901f)),
            abs, _mm_set1_ps(-0.0170881256f)),
            abs, _mm_set1_ps(0.0308918810f));
        const __m128 lo = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(-0.0501743046f),
            abs, _mm_set1_ps(0.0889789874f)),
            abs, _mm_set1_ps(-0.2145988016f)),
            abs, _mm_set1_ps(1.5707963050f));

        const __m128 result = _mm_madd_ps(hi, abs4, lo);
        return _mm_sel_ps(_mm_mul_ps(t1, result), _mm_nmsub_ps(t1, result, _mm_set1_ps(3.1415926535898f)), mask);
    }

    static __forceinline __m128 Sin(__m128 x)
    {
        const float CC0 = -0.0013602249f;
        const float CC1 = 0.0416566950f;
        const float CC2 = -0.4999990225f;
        const float SC0 = -0.0001950727f;
        const float SC1 = 0.0083320758f;
        const float SC2 = -0.1666665247f;
        const float KC1 = 1.57079625129f;
        const float KC2 = 7.54978995489e-8f;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i two = _mm_set1_epi32(2);
        const __m128i zero = _mm_setzero_si128();

        const __m128 x1 = _mm_mul_ps(x, _mm_set1_ps(0.63661977236f));
        const __m128i q = _mm_cvtps_epi32(x1);
        const __m128i offsetSin = _mm_and_si128(q, _mm_set1_epi32(3));
        const __m128 qf = _mm_cvtepi32_ps(q);
        const __m128 x11 = _mm_nmsub_ps(qf, _mm_set1_ps(KC2), _mm_nmsub_ps(qf, _mm_set1_ps(KC1), x));
        const __m128 x12 = _mm_mul_ps(x11, x11);
        const __m128 x13 = _mm_mul_ps(x12, x11);

        const __m128 cx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(CC0), x12, _mm_set1_ps(CC1)), x12, _mm_set1_ps(CC2)), x12, _mm_set1_ps(1.0f));
        const __m128 sx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(SC0), x12, _mm_set1_ps(SC1)), x12, _mm_set1_ps(SC2)), x13, x11);

        const __m128i sinMask = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, one), zero);
        const __m128 s1 = _mm_sel_ps(cx, sx, _mm_castsi128_ps(sinMask));

        const __m128i sinMask2 = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, two), zero);

        const __m128 highBitMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
        return _mm_sel_ps(_mm_xor_ps(highBitMask, s1), s1, _mm_castsi128_ps(sinMask2));
    }

    static __forceinline void Sincos(__m128 * RESTRICT sin, __m128 * RESTRICT cos, __m128 x)
    {
        const float CC0 = -0.0013602249f;
        const float CC1 = 0.0416566950f;
        const float CC2 = -0.4999990225f;
        const float SC0 = -0.0001950727f;
        const float SC1 = 0.0083320758f;
        const float SC2 = -0.1666665247f;
        const float KC1 = 1.57079625129f;
        const float KC2 = 7.54978995489e-8f;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i two = _mm_set1_epi32(2);
        const __m128i zero = _mm_setzero_si128();

        const __m128 x1 = _mm_mul_ps(x, _mm_set1_ps(0.63661977236f));
        const __m128i q = _mm_cvtps_epi32(x1);
        const __m128i offsetSin = _mm_and_si128(q, _mm_set1_epi32(3));
        const __m128i temp = _mm_add_epi32(one, offsetSin);
        const __m128i offsetCos = temp;
        const __m128 qf = _mm_cvtepi32_ps(q);
        const __m128 x11 = _mm_nmsub_ps(qf, _mm_set1_ps(KC2), _mm_nmsub_ps(qf, _mm_set1_ps(KC1), x));
        const __m128 x12 = _mm_mul_ps(x11, x11);
        const __m128 x13 = _mm_mul_ps(x12, x11);

        const __m128 cx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(CC0), x12, _mm_set1_ps(CC1)), x12, _mm_set1_ps(CC2)), x12, _mm_set1_ps(1.0f));
        const __m128 sx = _mm_madd_ps(_mm_madd_ps(_mm_madd_ps(_mm_set1_ps(SC0), x12, _mm_set1_ps(SC1)), x12, _mm_set1_ps(SC2)), x13, x11);

        const __m128i sinMask = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, one), zero);
        const __m128i cosMask = _mm_cmpeq_epi32(_mm_and_si128(offsetCos, one), zero);
        const __m128 s1 = _mm_sel_ps(cx, sx, _mm_castsi128_ps(sinMask));
        const __m128 c1 = _mm_sel_ps(cx, sx, _mm_castsi128_ps(cosMask));

        const __m128i sinMask2 = _mm_cmpeq_epi32(_mm_and_si128(offsetSin, two), zero);
        const __m128i cosMask2 = _mm_cmpeq_epi32(_mm_and_si128(offsetCos, two), zero);

        const __m128 highBitMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
        const __m128 s = _mm_sel_ps(_mm_xor_ps(highBitMask, s1), s1, _mm_castsi128_ps(sinMask2));
        const __m128 c = _mm_sel_ps(_mm_xor_ps(highBitMask, c1), c1, _mm_castsi128_ps(cosMask2));
        *sin = s;
        *cos = c;
    }

    void SoaQuatPos::Interpolate(const SoaQuatPos * RESTRICT p0, const SoaQuatPos * RESTRICT p1, float t)
    {
        const int numElements = this->mNumElements;

        assert(this->mNumElements != 0);
        assert(t >= 0 && t <= 1.0f);
        assert(p0 != NULL && p1 != NULL);
        assert(numElements == p0->mNumElements && numElements == p1->mNumElements);
        
        const __m128 t0 = _mm_set1_ps(t);
        const __m128 t1 = _mm_set1_ps(1.0f - t);

        float *pX = this->mX;
        float *pY = this->mY;
        float *pZ = this->mZ;
        float *pX0 = p0->mX;
        float *pY0 = p0->mY;
        float *pZ0 = p0->mZ;
        float *pX1 = p1->mX;
        float *pY1 = p1->mY;
        float *pZ1 = p1->mZ;

        for (int i = 0; i < numElements; i += SIMD_SIZE)
        {
            const __m128 x0 = _mm_load_ps(pX0 + i);
            const __m128 x1 = _mm_load_ps(pX1 + i);
            const __m128 y0 = _mm_load_ps(pY0 + i);
            const __m128 y1 = _mm_load_ps(pY1 + i);
            const __m128 z0 = _mm_load_ps(pZ0 + i);
            const __m128 z1 = _mm_load_ps(pZ1 + i);
            const __m128 x = _mm_add_ps(x0, _mm_mul_ps(t0, _mm_sub_ps(x1, x0)));
            const __m128 y = _mm_add_ps(y0, _mm_mul_ps(t0, _mm_sub_ps(y1, y0)));
            const __m128 z = _mm_add_ps(z0, _mm_mul_ps(t0, _mm_sub_ps(z1, z0)));
            _mm_store_ps(pX + i, x);
            _mm_store_ps(pY + i, y);
            _mm_store_ps(pZ + i, z);
        }

        float *pQx0 = p0->mQx;
        float *pQy0 = p0->mQy;
        float *pQz0 = p0->mQz;
        float *pQw0 = p0->mQw;
        float *pQx1 = p1->mQx;
        float *pQy1 = p1->mQy;
        float *pQz1 = p1->mQz;
        float *pQw1 = p1->mQw;
        float *pQx = this->mQx;
        float *pQy = this->mQy;
        float *pQz = this->mQz;
        float *pQw = this->mQw;

        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 zero = _mm_setzero_ps();
        const __m128 negOneBit = _mm_castsi128_ps(_mm_set1_epi32(0x80000000ul));
        const __m128 small = _mm_set1_ps(1e-6f);
        for (int i = 0; i < numElements; i += SIMD_SIZE)
        {
            const __m128 qx0 = _mm_load_ps(pQx0 + i);
            const __m128 qy0 = _mm_load_ps(pQy0 + i);
            const __m128 qz0 = _mm_load_ps(pQz0 + i);
            const __m128 qw0 = _mm_load_ps(pQw0 + i);
            const __m128 qx1 = _mm_load_ps(pQx1 + i);
            const __m128 qy1 = _mm_load_ps(pQy1 + i);
            const __m128 qz1 = _mm_load_ps(pQz1 + i);
            const __m128 qw1 = _mm_load_ps(pQw1 + i);

            const __m128 qx0qx1 = _mm_mul_ps(qx0, qx1);
            const __m128 qy0qy1 = _mm_mul_ps(qy0, qy1);
            const __m128 qz0qz1 = _mm_mul_ps(qz0, qz1);
            const __m128 qw0qw1 = _mm_mul_ps(qw0, qw1);

            __m128 cosOmega = _mm_add_ps(_mm_add_ps(_mm_add_ps(qx0qx1, qy0qy1), qz0qz1), qw0qw1);
            const __m128 mask = _mm_cmplt_ps(cosOmega, zero);
            const __m128 toX = _mm_sel_ps(qx1, _mm_or_ps(qx1, negOneBit), mask);
            const __m128 toY = _mm_sel_ps(qy1, _mm_or_ps(qy1, negOneBit), mask);
            const __m128 toZ = _mm_sel_ps(qz1, _mm_or_ps(qz1, negOneBit), mask);
            const __m128 toW = _mm_sel_ps(qw1, _mm_or_ps(qw1, negOneBit), mask);
            cosOmega = _mm_sel_ps(cosOmega, _mm_or_ps(cosOmega, negOneBit), mask);

            const __m128 oneMinusCosOmega = _mm_sub_ps(one, cosOmega);
            const __m128 closeToOneMask = _mm_cmpgt_ps(oneMinusCosOmega, small);

            const __m128 omega = Acos(cosOmega);
            const __m128 invSinOmega = _mm_rcp_ps(Sin(omega));
            const __m128 t0a = _mm_mul_ps(Sin(_mm_mul_ps(t1, omega)), invSinOmega);
            const __m128 t1a = _mm_mul_ps(Sin(_mm_mul_ps(t0, omega)), invSinOmega);

            const __m128 t0b = t1;
            const __m128 t1b = t0;
           
            const __m128 t0 = _mm_sel_ps(t0b, t0a, closeToOneMask);
            const __m128 t1 = _mm_sel_ps(t1b, t1a, closeToOneMask);

            const __m128 x = _mm_add_ps(_mm_mul_ps(t0, qx0), _mm_mul_ps(t1, toX));
            const __m128 y = _mm_add_ps(_mm_mul_ps(t0, qy0), _mm_mul_ps(t1, toY));
            const __m128 z = _mm_add_ps(_mm_mul_ps(t0, qz0), _mm_mul_ps(t1, toZ));
            const __m128 w = _mm_add_ps(_mm_mul_ps(t0, qw0), _mm_mul_ps(t1, toW));

            _mm_store_ps(pQx + i, x);
            _mm_store_ps(pQy + i, y);
            _mm_store_ps(pQz + i, z);
            _mm_store_ps(pQw + i, w);
        }
    }

    void SoaQuatPos::ConvertToMat4x3(Mat4x3 *matrices) const
    {
        float *pQx = this->mQx;
        float *pQy = this->mQy;
        float *pQz = this->mQz;
        float *pQw = this->mQw;
        float *pX = this->mX;
        float *pY = this->mY;
        float *pZ = this->mZ;

        for (int i = 0, e = this->mNumElements; i < e; ++i, ++matrices)
        {
            float qx = pQx[i];
            float qy = pQy[i];
            float qz = pQz[i];
            float qw = pQw[i];
            float x = pX[i];
            float y = pY[i];
            float z = pZ[i];

            float qxqx = qx * qx;
            float qxqy = qx * qy;
            float qxqz = qx * qz;
            float qxqw = qx * qw;

            float qyqy = qy * qy;
            float qyqz = qy * qz;
            float qyqw = qy * qw;
            
            float qzqz = qz * qz;
            float qzqw = qz * qw;

            matrices->v[0] = 1.0f - 2.0f * (qyqy + qzqz);
            matrices->v[1] =        2.0f * (qxqy - qzqw);
            matrices->v[2] =        2.0f * (qxqz + qyqw);
            matrices->v[3] = x;

            matrices->v[4] =        2.0f * (qxqy + qzqw);
            matrices->v[5] = 1.0f - 2.0f * (qxqx + qzqz);
            matrices->v[6] =        2.0f * (qyqz - qxqw);
            matrices->v[7] = y;
            
            matrices->v[8] =        2.0f * (qxqz - qyqw);
            matrices->v[9] =        2.0f * (qyqz + qxqw);
            matrices->v[10] = 1.0f - 2.0f * (qxqx + qyqy);
            matrices->v[11] = z;
        }
    }

    Quaternion UncompressQuaternion(const CompressedQuaternion &cq)
    {
        Quaternion q = { cq.x, cq.y, cq.z, sqrt(fabs(1.0f - (cq.x * cq.x + cq.y * cq.y + cq.z * cq.z))) };
        return q;
    }

    void TestMath()
    {
        for (int i = -630; i < 630; ++i)
        {
            const float f = static_cast<float>(i) / 100.0f;
            __m128 s, c;
            Sincos(&s, &c, _mm_set1_ps(f));

            const __m128 testSin = _mm_set1_ps(sinf(f));
            const __m128 testCos = _mm_set1_ps(cosf(f));

            const __m128 sinError = Abs(_mm_sub_ps(testSin, s));
            const __m128 cosError = Abs(_mm_sub_ps(testCos, c));

            const __m128 acceptableError = _mm_set1_ps(0.00001f);
            const __m128 sinMask = _mm_cmpge_ps(sinError, acceptableError);
            const __m128 cosMask = _mm_cmpge_ps(cosError, acceptableError);

            for (int i = 0; i < 4; ++i)
            {
                assert(sinMask.m128_u32[i] == 0);
                assert(cosMask.m128_u32[i] == 0);
            }
        }
    }
}

// Portions (Sqrt, Acos, Sincos) courtesy of Bullet Physics, which is licensed as follows:

/*
   Copyright (C) 2006, 2007 Sony Computer Entertainment Inc.
   All rights reserved.

   Redistribution and use in source and binary forms,
   with or without modification, are permitted provided that the
   following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Sony Computer Entertainment Inc nor the names
      of its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
