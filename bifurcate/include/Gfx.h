#ifndef BIFURCATE_GFX_H
#define BIFURCATE_GFX_H

#include "MathTypes.h"

namespace bg
{
    struct IndexBuffer;
    struct VertexBuffer;
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
    void *GfxGetWindowHandle();

    IndexBuffer *IndexBufferCreate(int numIndices, unsigned short *indices);
    VertexBuffer *VertexBufferCreate(int numVertices, size_t vertexSize, const void *vertices);
    MeshWeightedPositionBuffer *MeshWeightedPositionBufferCreate(int numPositions, Vec4 *weightedPositions, unsigned char *jointIndices);
    void DrawSkinnedMesh(const SkinnedMeshData *meshData, const SoaQuatPos *poseData);

    const int MAX_NUM_JOINTS = 256;
}

#endif
