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

    struct MeshData
    {
        int mNumJoints;
        int mNumMeshes;
        Joint *mJoints;
        int *mNumVerts;
        IndexBuffer **mIndexBuffers;
        MeshVertexBuffer **mVertexBuffers;
        MeshWeightedPositionBuffer **mWeightedPositionBuffers;
    };

    const MeshData *LoadMesh(const char *fileName);
}

#endif
