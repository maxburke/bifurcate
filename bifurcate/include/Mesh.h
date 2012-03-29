#ifndef BIFURCATE_MESH_H
#define BIFURCATE_MESH_H

#include "Config.h"
#include "MathTypes.h"
#include "Gfx.h"

namespace bg
{
    struct Joint
    {
        const char *mName;
        uint64_t mNameHash;
        QuatPos mInitial;
    };

    struct SkinnedMeshData
    {
        int mNumJoints;
        int mNumMeshes;
        Joint *mJoints;
        int *mNumTris;
        IndexBuffer **mIndexBuffers;
        VertexBuffer **mVertexBuffers;
        MeshWeightedPositionBuffer **mWeightedPositionBuffers;
        Mat4x4 *mBindPose;
        Mat4x4 *mInverseBindPose;
    };

    const SkinnedMeshData *LoadMesh(const char *fileName);
}

#endif
