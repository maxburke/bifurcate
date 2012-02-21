#ifndef BIFURCATE_MATH_H
#define BIFURCATE_MATH_H

#pragma warning(push)
#pragma warning(disable:4201)

namespace bg
{
    struct Vec2
    {
        union 
        {
            struct
            {
                float x;
                float y;
            };
            float v[2];
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
            };
            float v[3];
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
