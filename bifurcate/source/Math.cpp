#include <math.h>
#include <string.h>
#include "MathTypes.h"
#include "Core.h"
#include "SseMath.h"

namespace bg
{
    Quaternion QuaternionUncompress(const CompressedQuaternion &cq)
    {
        Quaternion q = { cq.x, cq.y, cq.z, sqrt(fabs(1.0f - (cq.x * cq.x + cq.y * cq.y + cq.z * cq.z))) };
        return q;
    }

    Quaternion QuaternionMultiply(const Quaternion * __restrict quat1, const Quaternion * __restrict quat2)
    {
        Quaternion result;
        const __m128 q1 = _mm_load_ps(quat1->v);
        const __m128 q2 = _mm_load_ps(quat2->v);

        // x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y)
        // y = (q1.w * q2.y) + (q1.y * q2.w) + (q1.z * q2.x) - (q1.x * q2.z)
        // z = (q1.w * q2.z) + (q1.z * q2.w) + (q1.x * q2.y) - (q1.y * q2.x)
        // w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z)

        __m128 negW = _mm_castsi128_ps(_mm_set_epi32(0x80000000, 0, 0, 0));

        __m128 q1w = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 qa = _mm_mul_ps(q1w, q2);

        __m128 q1xyzx = _mm_or_ps(_mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 2, 1, 0)), negW);
        __m128 q2wwwx = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 3, 3, 3));
        __m128 qb = _mm_mul_ps(q1xyzx, q2wwwx);

        __m128 q1yzxy = _mm_or_ps(_mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 0, 2, 1)), negW);
        __m128 q2zxyy = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 1, 0, 2));
        __m128 qc = _mm_mul_ps(q1yzxy, q2zxyy);

        __m128 q1zxyz = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 1, 0, 2));
        __m128 q2yzxz = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 0, 2, 1));
        __m128 qd = _mm_mul_ps(q1zxyz, q2yzxz);
        
        const __m128 q = _mm_add_ps(qa, _mm_add_ps(qb, _mm_sub_ps(qc, qd)));
        _mm_store_ps(result.v, q);
        return result;
    }

    Quaternion QuaternionFromAxisAngle(const Vec3 * __restrict axis, float angle)
    {
        assert(axis->__w == 0);
        __m128 sin, cos;
        _mm_sincos_ps(&sin, &cos, _mm_set1_ps(angle / 2));

        __m128 axisVector = _mm_load_ps(axis->v);
        __m128 lengthSquared = _mm_dot_ps(axisVector, axisVector);
        __m128 normalizedAxis = _mm_mul_ps(_mm_mul_ps(axisVector, _mm_rsqrt_ps(lengthSquared)), sin);
        __m128 quatVector = 
            _mm_or_ps(_mm_and_ps(_mm_castsi128_ps(_mm_set_epi32(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)), normalizedAxis),
                  _mm_and_ps(_mm_castsi128_ps(_mm_set_epi32(0xFFFFFFFF, 0, 0, 0)), cos));

        Quaternion q;
        _mm_store_ps(&q.x, quatVector);
        return q;
    }

    QuatPos::QuatPos(float px, float py, float pz, float qx, float qy, float qz)
    {
        Vec3 compressedQuaternion = { qx, qy, qz, 0 };
        quat = QuaternionUncompress(compressedQuaternion);
        pos.x = px;
        pos.y = py;
        pos.z = pz;
    }

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
            const __m128 abs = _mm_abs_ps(oneMinus);
            const __m128 w = _mm_sqrt_ps(abs);
            _mm_store_ps(qw + i, w);
        }
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

            const __m128 omega = _mm_acos_ps(cosOmega);
            const __m128 invSinOmega = _mm_rcp_ps(_mm_sin_ps(omega));
            const __m128 t0a = _mm_mul_ps(_mm_sin_ps(_mm_mul_ps(t1, omega)), invSinOmega);
            const __m128 t1a = _mm_mul_ps(_mm_sin_ps(_mm_mul_ps(t0, omega)), invSinOmega);

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

    void SoaQuatPos::ConvertToMat4x4(Mat4x4 *matrices) const
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

            matrices->v[12] = 0;
            matrices->v[13] = 0;
            matrices->v[14] = 0;
            matrices->v[15] = 1;
        }
    }

    Mat4x4 *Mat4x4FromQuaternion(Mat4x4 * __restrict mat, const Quaternion *__restrict q)
    {
        const float * __restrict quat = q->v;
        float * __restrict out = mat->v;

        float qx = quat[0];
        float qy = quat[1];
        float qz = quat[2];
        float qw = quat[3];

        float qxqx = qx * qx;
        float qxqy = qx * qy;
        float qxqz = qx * qz;
        float qxqw = qx * qw;

        float qyqy = qy * qy;
        float qyqz = qy * qz;
        float qyqw = qy * qw;
            
        float qzqz = qz * qz;
        float qzqw = qz * qw;

        out[0] = 1.0f - 2.0f * (qyqy + qzqz);
        out[1] =        2.0f * (qxqy - qzqw);
        out[2] =        2.0f * (qxqz + qyqw);
        out[3] = 0;

        out[4] =        2.0f * (qxqy + qzqw);
        out[5] = 1.0f - 2.0f * (qxqx + qzqz);
        out[6] =        2.0f * (qyqz - qxqw);
        out[7] = 0;
            
        out[8] =        2.0f * (qxqz - qyqw);
        out[9] =        2.0f * (qyqz + qxqw);
        out[10] = 1.0f - 2.0f * (qxqx + qyqy);
        out[11] = 0;

        out[12] = 0;
        out[13] = 0;
        out[14] = 0;
        out[15] = 1;

        return mat;
    }

    Mat4x4 *Mat4x4FromTranslation(Mat4x4 *__restrict mat, const Vec3 * __restrict translation)
    {
        float * __restrict out = mat->v;
        const float x = translation->x;
        const float y = translation->y;
        const float z = translation->z;
        out[0] = 1; out[1] = 0; out[2] = 0; out[3] = 0;
        out[4] = 0; out[5] = 1; out[6] = 0; out[7] = 0;
        out[8] = 0; out[9] = 0; out[10] = 1; out[11] = 0;
        out[12] = x; out[13] = y; out[14] = z; out[15] = 1;

        return mat;
    }

    Mat4x4 *Mat4x4FromQuatPos(Mat4x4 * __restrict mat, const QuatPos * __restrict quatPos)
    {
        Mat4x4 rotation;
        const float * __restrict pos = quatPos->pos.v;
        Mat4x4FromQuaternion(&rotation, &quatPos->quat);

        Vec3 localTranslation = { pos[0], pos[1], pos[2], 1 };
        Mat4x4 translation;
        return Mat4x4Multiply(mat, &rotation, Mat4x4FromTranslation(&translation, &localTranslation));
    }

    Mat4x4 *Mat4x4Invert(Mat4x4 * __restrict out, const Mat4x4 * __restrict in)
    {
        // This matrix inversion routine is from Intel's P3 optimization material.
        // ftp://download.intel.com/design/PentiumIII/sml/24504301.pdf

        const float * __restrict src = in->v;
        float * __restrict dst = out->v;

        __m128 minor0, minor1, minor2, minor3;
        __m128 row0,   row1,   row2,   row3;
        __m128 det,    tmp1;
        tmp1 = _mm_setzero_ps();
        row1 = _mm_setzero_ps();
        row3 = _mm_setzero_ps();
        tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src+ 4));
        row1 = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src+8)), (__m64*)(src+12));
        row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
        row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
        tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
        row3 = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src+10)), (__m64*)(src+14));
        row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
        row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);
    
        tmp1 = _mm_mul_ps(row2, row3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        minor0 = _mm_mul_ps(row1, tmp1);
        minor1 = _mm_mul_ps(row0, tmp1);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
        minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
        minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
        minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);
    
        tmp1 = _mm_mul_ps(row1, row2);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
        minor3 = _mm_mul_ps(row0, tmp1);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
        minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
        minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
        minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);
    
        tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        row2 = _mm_shuffle_ps(row2, row2, 0x4E);
        minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
        minor2 = _mm_mul_ps(row0, tmp1);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
        minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
        minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
        minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);
    
        tmp1 = _mm_mul_ps(row0, row1);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
        minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
        minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
        minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
    
        tmp1 = _mm_mul_ps(row0, row3);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
        minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
        minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
        minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
    
        tmp1 = _mm_mul_ps(row0, row2);
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
        minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
        minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
        tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
        minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
        minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
    
        det = _mm_mul_ps(row0, minor0);
        det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
        det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
        tmp1 = _mm_rcp_ss(det);
        det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
        det = _mm_shuffle_ps(det, det, 0x00);
        minor0 = _mm_mul_ps(det, minor0);
        _mm_storel_pi((__m64*)(dst), minor0);
        _mm_storeh_pi((__m64*)(dst+2), minor0);
        minor1 = _mm_mul_ps(det, minor1);
        _mm_storel_pi((__m64*)(dst+4), minor1);
        _mm_storeh_pi((__m64*)(dst+6), minor1);
        minor2 = _mm_mul_ps(det, minor2);
        _mm_storel_pi((__m64*)(dst+ 8), minor2);
        _mm_storeh_pi((__m64*)(dst+10), minor2);
        minor3 = _mm_mul_ps(det, minor3);
        _mm_storel_pi((__m64*)(dst+12), minor3);
        _mm_storeh_pi((__m64*)(dst+14), minor3);

        return out;
    }

    __forceinline Mat4x4 *Mat4x4Multiply(Mat4x4 * __restrict out, const Mat4x4 * __restrict lhs, const Mat4x4 * __restrict rhs)
    {
        const float * __restrict l = lhs->v;
        const float * __restrict r = rhs->v;
        float * __restrict o = out->v;

        __m128 p0 = _mm_load_ps(l + 0);
        __m128 p1 = _mm_load_ps(l + 4);
        __m128 p2 = _mm_load_ps(l + 8);
        __m128 p3 = _mm_load_ps(l + 12);

        __m128 i0 = _mm_load_ps(r + 0);
        __m128 i1 = _mm_load_ps(r + 4);
        __m128 i2 = _mm_load_ps(r + 8);
        __m128 i3 = _mm_load_ps(r + 12);

        _MM_TRANSPOSE4_PS(i0, i1, i2, i3);

        _mm_store_ss(o + 0, _mm_dot_ps(p0, i0));
        _mm_store_ss(o + 1, _mm_dot_ps(p0, i1));
        _mm_store_ss(o + 2, _mm_dot_ps(p0, i2));
        _mm_store_ss(o + 3, _mm_dot_ps(p0, i3));

        _mm_store_ss(o + 4, _mm_dot_ps(p1, i0));
        _mm_store_ss(o + 5, _mm_dot_ps(p1, i1));
        _mm_store_ss(o + 6, _mm_dot_ps(p1, i2));
        _mm_store_ss(o + 7, _mm_dot_ps(p1, i3));

        _mm_store_ss(o + 8, _mm_dot_ps(p2, i0));
        _mm_store_ss(o + 9, _mm_dot_ps(p2, i1));
        _mm_store_ss(o + 10, _mm_dot_ps(p2, i2));
        _mm_store_ss(o + 11, _mm_dot_ps(p2, i3));

        _mm_store_ss(o + 12, _mm_dot_ps(p3, i0));
        _mm_store_ss(o + 13, _mm_dot_ps(p3, i1));
        _mm_store_ss(o + 14, _mm_dot_ps(p3, i2));
        _mm_store_ss(o + 15, _mm_dot_ps(p3, i3));

        return out;
    }

    void MultiplyInverseBindPose(Mat4x4 * __restrict matrices, int numPoses, const Mat4x4 * __restrict poseMatrices, const Mat4x4 * __restrict inverseBindPose)
    {
        for (int i = 0; i < numPoses; ++i)
        {
            Mat4x4Multiply(matrices + i, poseMatrices + i, inverseBindPose + i);
        }
    }

    void TestMath()
    {
        for (int i = -630; i < 630; ++i)
        {
            const float f = static_cast<float>(i) / 100.0f;
            __m128 s, c;
            _mm_sincos_ps(&s, &c, _mm_set1_ps(f));

            const __m128 testSin = _mm_set1_ps(sinf(f));
            const __m128 testCos = _mm_set1_ps(cosf(f));

            const __m128 sinError = _mm_abs_ps(_mm_sub_ps(testSin, s));
            const __m128 cosError = _mm_abs_ps(_mm_sub_ps(testCos, c));

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
