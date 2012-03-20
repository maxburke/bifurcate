#ifndef BIFURCATE_MATH_H
#define BIFURCATE_MATH_H

#pragma warning(push)
#pragma warning(disable:4201)

namespace bg
{
    __declspec(align(16)) struct Vec2
    {
        union 
        {
            struct
            {
                float x;
                float y;
                float __z;
                float __w;
            };
            float v[4];
        };
    };

    __declspec(align(16)) struct Vec3
    {
        union 
        {
            struct
            {
                float x;
                float y;
                float z;
                float __w;
            };
            float v[4];
        };
    };

    __declspec(align(16)) struct Vec4
    {
        union 
        {
            struct
            {
                float x;
                float y;
                float z;
                float w;
            };
            float v[4];
        };
    };

    __declspec(align(16)) struct Mat4x3
    {
        float v[12];

        void SetIdentity();
    };

    __declspec(align(16)) struct Mat4x4
    {
        float v[16];

        void SetIdentity();
    };

    typedef Vec4 Plane;
    typedef Vec4 Quaternion;
    typedef Vec3 CompressedQuaternion;

    struct QuatPos
    {
        QuatPos() {}
        QuatPos(float px, float py, float pz, float qx, float qy, float qz);
        Quaternion quat;
        Vec3 pos;
    };

    struct SoaQuatPos
    {
        SoaQuatPos()
            : mNumElements(0), mBase(0), mX(0), mY(0), mZ(0), mQx(0), mQy(0), mQz(0), mQw(0)
        {}

        SoaQuatPos(int numElements, void *memory)
        {
            Initialize(numElements, memory);
        }

        void UncompressQw();
        void Initialize(int numElements, void *memory);
        static size_t MemorySize(int numElements);
        void Interpolate(const SoaQuatPos *p0, const SoaQuatPos *p1, float t);
        void ConvertToMat4x4(Mat4x4 *matrices) const;

        int mNumElements;
        float *mBase;
        float *mX;
        float *mY;
        float *mZ;
        float *mQx;
        float *mQy;
        float *mQz;
        float *mQw;
    };

    struct BBox
    {
        BBox(const Vec3 &_min, const Vec3 &_max)
            : min(_min),
              max(_max)
        {}

        Vec3 min;
        Vec3 max;
    };

    Quaternion QuaternionUncompress(const CompressedQuaternion &cq);
    Quaternion QuaternionFromAxisAngle(const Vec3 * __restrict axis, float angle);
    Quaternion QuaternionMultiply(const Quaternion * __restrict q1, const Quaternion * __restrict q2);
    void Mat4x4Multiply(Mat4x4 * __restrict out, const Mat4x4 * __restrict lhs, const Mat4x4 * __restrict rhs);
    void Mat4x4FromQuaternion(Mat4x4 * __restrict mat, const Quaternion *__restrict q);
    void Mat4x4FromQuatPos(Mat4x4 * __restrict out, const QuatPos * __restrict in);
    void Mat4x4Invert(Mat4x4 * __restrict out, const Mat4x4 * __restrict in);
    void MultiplyInverseBindPose(Mat4x4 * __restrict matrices, int numPoses, const Mat4x4 * __restrict poseMatrices, const Mat4x4 * __restrict inverseBindPose);
}

#pragma warning(pop)

#endif
