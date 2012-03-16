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
        float mTexU;
        float mTexV;
        short mWeightIndex;
        short mWeightElement;
    };

    struct WeightedPosition
    {
        Vec3 mPosition;
        float mJointWeight;
        int mJointIndex;
    };

    bool GfxInitialize(void *instance, int width, int height);
    void GfxShutdown();
    void GfxBeginScene();
    void GfxEndScene();

    IndexBuffer *IndexBufferCreate(int numIndices, unsigned short *indices);
    MeshVertexBuffer *MeshVertexBufferCreate(int numVertices, MeshVertex *vertices);
    MeshWeightedPositionBuffer *MeshWeightedPositionBufferCreate(int numPositions, Vec4 *weightedPositions, unsigned char *jointIndices);
    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const SoaQuatPos *poseData);

    const int MAX_NUM_JOINTS = 256;
}

#endif
