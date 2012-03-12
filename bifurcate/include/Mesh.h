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
        MeshVertexBuffer **mVertexBuffers;
        MeshWeightedPositionBuffer **mWeightedPositionBuffers;
    };

    const SkinnedMeshData *LoadMesh(const char *fileName);
}

#endif
