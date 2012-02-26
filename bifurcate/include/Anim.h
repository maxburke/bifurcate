#ifndef BIFURCATE_ANIM_H
#define BIFURCATE_ANIM_H

#include "Config.h"
#include "MathTypes.h"

namespace bg
{
    enum AnimFlags
    {
        ANIM_TX = 1 << 0,
        ANIM_TY = 1 << 1,
        ANIM_TZ = 1 << 2,
        ANIM_QX = 1 << 3,
        ANIM_QY = 1 << 4,
        ANIM_QZ = 1 << 5,
    };

    struct AnimJoint
    {
        const char *mName;
        uint64_t mNameHash;
        int mParentIndex;
        int mJointFlags;
        int mFirstComponent;
    };

    struct AnimData
    {
        int mNumFrames;
        int mNumJoints;
        int mNumAnimatedComponents;
        int mFrameRate;
        AnimJoint *mJoints;
        BBox *mBoundingBoxes;
        QuatPos *mBaseFrame;
        float *mComponentFrames;
    };

    const AnimData *LoadAnim(const char *fileName);
}

#endif
