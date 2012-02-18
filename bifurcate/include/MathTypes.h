#ifndef BIFURCATE_MATH_H
#define BIFURCATE_MATH_H

#pragma warning(push)
#pragma warning(disable:4201)

namespace bg
{
    struct vec2
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

        vec2() {}
        vec2(float x, float y)
        {
            v[0] = x;
            v[1] = y;
        }
    };

    struct vec3
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

        vec3() {}
        vec3(float x, float y, float z)
        {
            v[0] = x;
            v[1] = y;
            v[2] = z;
        }
    };

    struct vec4
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

        vec4() {}
        vec4(float x, float y, float z, float w)
        {
            v[0] = x;
            v[1] = y;
            v[2] = z;
            v[3] = w;
        }
    };


    typedef vec4 plane;
    typedef vec4 quaternion;
    typedef vec3 compressed_quaternion;

    quaternion uncompress_quaternion(const compressed_quaternion &cq);

    struct quat_pos
    {
        quat_pos() {}
        quat_pos(float px, float py, float pz, float qx, float qy, float qz)
            : quat(uncompress_quaternion(compressed_quaternion(qx, qy, qz))),
              pos(px, py, pz)
        { }

        quaternion quat;
        vec3 pos;
    };

    struct bbox
    {
        bbox(const vec3 &_min, const vec3 &_max)
            : min(_min),
              max(_max)
        {}

        vec3 min;
        vec3 max;
    };

}

#pragma warning(pop)

#endif
