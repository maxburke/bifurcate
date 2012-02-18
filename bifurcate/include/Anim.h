#ifndef BIFURCATE_ANIM_H
#define BIFURCATE_ANIM_H

#include "Config.h"
#include "MathTypes.h"

namespace bg
{
    enum anim_flags_t
    {
        ANIM_TX = 1 << 0,
        ANIM_TY = 1 << 1,
        ANIM_TZ = 1 << 2,
        ANIM_QX = 1 << 3,
        ANIM_QY = 1 << 4,
        ANIM_QZ = 1 << 5,
    };

    struct anim_joint
    {
        const char *mName;
        uint64_t mNameHash;
        int mParentIndex;
        int mJointFlags;
        int mFirstComponent;
    };

    struct anim_data
    {
        int mNumFrames;
        int mNumJoints;
        int mNumAnimatedComponents;
        int mFrameRate;
        anim_joint *mJoints;
        bbox *mBoundingBoxes;
        quat_pos *mBaseFrame;
        float *mComponentFrames;
    };

    const anim_data *load_anim(const char *file_name);
}

#endif
