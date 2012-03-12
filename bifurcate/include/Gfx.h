#ifndef BIFURCATE_GFX_H
#define BIFURCATE_GFX_H

#include "MathTypes.h"

namespace bg
{
    struct IndexBuffer;
    struct MeshVertexBuffer;
    struct MeshWeightedPositionBuffer;
    struct Material;
    struct SkinnedMeshData;

    struct MeshVertex
    {
        Vec2 mTexCoords;
        short mWeightIndex;
        short mWeightElement;
    };

    struct WeightedPosition
    {
        Vec3 mPosition;
        float mJointWeight;
        int mJointIndex;
    };

    IndexBuffer *IndexBufferCreate(int numIndices, unsigned short *indices);
    MeshVertexBuffer *MeshVertexBufferCreate(int numVertices, MeshVertex *vertices);
    MeshWeightedPositionBuffer *MeshWeightedPositionBufferCreate(int numPositions, Vec4 *weightedPositions, unsigned char *jointIndices);

    int GfxInitialize(void *instance, int width, int height);
    void GfxShutdown();
    void GfxBeginScene();
    void GfxEndScene();

    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const SoaQuatPos *poseData);
}

#endif
