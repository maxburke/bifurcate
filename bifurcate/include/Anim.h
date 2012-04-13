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
        ANIM_TRANSLATION_MASK = (ANIM_TX | ANIM_TY | ANIM_TZ),
        ANIM_QX = 1 << 3,
        ANIM_QY = 1 << 4,
        ANIM_QZ = 1 << 5,
        ANIM_ROTATION_MASK = (ANIM_QX | ANIM_QY | ANIM_QZ),
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
        float mInvClockTicksPerFrame;
        AnimJoint *mJoints;
        intptr_t *mComponentIndices;
        BBox *mBoundingBoxes;
        SoaQuatPos mBaseFrame;
        float *mComponentFrames;
    };

    const AnimData *LoadAnim(const char *fileName);
    void InterpolateAnimationFrames(SoaQuatPos *interpolated, const AnimData *animData, int frame1, int frame2, float lerp);
}

#endif
