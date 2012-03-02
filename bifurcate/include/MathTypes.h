#ifndef BIFURCATE_MATH_H
#define BIFURCATE_MATH_H

#pragma warning(push)
#pragma warning(disable:4201)

namespace bg
{
    union ConversionUnion
    {
        unsigned int mUnsigned;
        int mSigned;
        float mFloat;
    };

    struct Vec2
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

        Vec2() {}
        Vec2(float x, float y)
        {
            v[0] = x;
            v[1] = y;
        }
    };

    struct Vec3
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

        Vec3() {}
        Vec3(float x, float y, float z)
        {
            v[0] = x;
            v[1] = y;
            v[2] = z;
        }
    };

    struct Vec4
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

        Vec4() {}
        Vec4(float x, float y, float z, float w)
        {
            v[0] = x;
            v[1] = y;
            v[2] = z;
            v[3] = w;
        }
    };


    typedef Vec4 Plane;
    typedef Vec4 Quaternion;
    typedef Vec3 CompressedQuaternion;

    Quaternion UncompressQuaternion(const CompressedQuaternion &cq);

    struct QuatPos
    {
        QuatPos() {}
        QuatPos(float px, float py, float pz, float qx, float qy, float qz)
            : quat(UncompressQuaternion(CompressedQuaternion(qx, qy, qz))),
              pos(px, py, pz)
        { }

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
        void Interpolate(SoaQuatPos *p0, SoaQuatPos *p1, float t);

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

}

#pragma warning(pop)

#endif
