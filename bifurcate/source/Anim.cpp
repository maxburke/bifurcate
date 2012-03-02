#include <new>
#include <string.h>

#include "Anim.h"
#include "Parser.h"
#include "Core.h"

namespace bg
{
    const AnimData *LoadAnim(const char *fileName)
    {
        using namespace bc;
        using namespace bx;

        #define CHOMP(string) if (!parser.ExpectAndDiscard(string)) return NULL; else (void)0

        AutoMemMap file(fileName);

        if (!file.Valid())
            return NULL;

        Parser parser(static_cast<const char *>(file.Mem()), static_cast<const char *>(file.Mem()) + file.Size());
        CHOMP("MD5Version");
        if (parser.ParseInt() != 10) return NULL;
        CHOMP("commandline");
        /* ParsedString commandLine = */ parser.ParseString();

        AnimData *ad = static_cast<AnimData *>(MemAlloc(POOL_ANIM, sizeof(AnimData)));

        CHOMP("numFrames");
        ParsedInt numFrames = parser.ParseInt();
        CHOMP("numJoints");
        ParsedInt numJoints = parser.ParseInt();
        CHOMP("frameRate");
        ParsedInt frameRate = parser.ParseInt();
        CHOMP("numAnimatedComponents");
        ParsedInt numAnimatedComponents = parser.ParseInt();

        if (!numFrames.Valid()
            || !numJoints.Valid()
            || !frameRate.Valid()
            || !numAnimatedComponents.Valid())
            return NULL;

        const uint64_t frequency = bc::GetFrequency();
        const float clockTicksPerFrame = static_cast<float>(frequency) / static_cast<float>(frameRate);

        ad->mNumFrames = numFrames;
        ad->mNumJoints = numJoints;
        ad->mFrameRate = frameRate;
        ad->mClockTicksPerFrame = clockTicksPerFrame;
        ad->mNumAnimatedComponents = numAnimatedComponents;
        ad->mJoints = static_cast<AnimJoint *>(MemAlloc(POOL_ANIM, sizeof(AnimJoint) * ad->mNumJoints));
        ad->mBoundingBoxes = static_cast<BBox *>(MemAlloc(POOL_ANIM, sizeof(BBox) * ad->mNumFrames));

        const size_t soaQuatPosMemorySize = SoaQuatPos::MemorySize(numJoints);
        void *soaQuatPosMemory = MemAlignedAlloc(POOL_ANIM, SIMD_ALIGNMENT, soaQuatPosMemorySize);
        ad->mBaseFrame.Initialize(numJoints, soaQuatPosMemory);
        ad->mComponentFrames = static_cast<float *>(MemAlloc(POOL_ANIM, sizeof(float) * ad->mNumAnimatedComponents * ad->mNumFrames));
        ad->mComponentIndices = static_cast<int *>(MemAlloc(POOL_ANIM, sizeof(int) * ad->mNumAnimatedComponents));

        CHOMP("hierarchy");
        CHOMP("{");

        AnimJoint *joints = ad->mJoints;
        int *componentIndexPtr = ad->mComponentIndices;
        SoaQuatPos *baseFrame = &ad->mBaseFrame;
        for (int i = 0, e = numJoints; i < e; ++i)
        {
            ParsedString jointName = parser.ParseString();
            ParsedInt parentIndex = parser.ParseInt();
            ParsedInt jointFlags = parser.ParseInt();
            ParsedInt firstComponent = parser.ParseInt();

            if (!jointName.Valid()
                || !parentIndex.Valid()
                || !jointFlags.Valid()
                || !firstComponent.Valid())
                return NULL;

            if (!Intern(&joints[i].mName, &joints[i].mNameHash, jointName.mBegin, jointName.mEnd))
                return NULL;

            joints[i].mJointFlags = jointFlags;
            joints[i].mParentIndex = parentIndex;
            joints[i].mFirstComponent = firstComponent;

            assert(jointFlags == 0 || (componentIndexPtr - ad->mComponentIndices == firstComponent));

            const int translationMask = jointFlags & ANIM_TRANSLATION_MASK;
            switch (translationMask)
            {
            case ANIM_TX:
                *componentIndexPtr++ = &baseFrame->mX[i] - baseFrame->mBase; break;
            case ANIM_TY:
                *componentIndexPtr++ = &baseFrame->mY[i] - baseFrame->mBase; break;
            case ANIM_TZ:
                *componentIndexPtr++ = &baseFrame->mZ[i] - baseFrame->mBase; break;
            case (ANIM_TX | ANIM_TY):
                *componentIndexPtr++ = &baseFrame->mX[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mY[i] - baseFrame->mBase; break;
            case (ANIM_TX | ANIM_TZ):
                *componentIndexPtr++ = &baseFrame->mX[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mZ[i] - baseFrame->mBase; break;
            case (ANIM_TY | ANIM_TZ):
                *componentIndexPtr++ = &baseFrame->mY[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mZ[i] - baseFrame->mBase; break;
            case (ANIM_TX | ANIM_TY | ANIM_TZ):
                *componentIndexPtr++ = &baseFrame->mX[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mY[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mZ[i] - baseFrame->mBase; break;
                break;
            }

            int rotationMask = jointFlags & ANIM_ROTATION_MASK;
            switch (rotationMask)
            {
            case ANIM_QX:
                *componentIndexPtr++ = &baseFrame->mQx[i] - baseFrame->mBase; break;
            case ANIM_QY:
                *componentIndexPtr++ = &baseFrame->mQy[i] - baseFrame->mBase; break;
            case ANIM_QZ:
                *componentIndexPtr++ = &baseFrame->mQz[i] - baseFrame->mBase; break;
            case (ANIM_QX | ANIM_QY):
                *componentIndexPtr++ = &baseFrame->mQx[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mQy[i] - baseFrame->mBase; break;
            case (ANIM_QX | ANIM_QZ):
                *componentIndexPtr++ = &baseFrame->mQx[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mQz[i] - baseFrame->mBase; break;
            case (ANIM_QY | ANIM_QZ):
                *componentIndexPtr++ = &baseFrame->mQy[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mQz[i] - baseFrame->mBase; break;
            case (ANIM_QX | ANIM_QY | ANIM_QZ):
                *componentIndexPtr++ = &baseFrame->mQx[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mQy[i] - baseFrame->mBase;
                *componentIndexPtr++ = &baseFrame->mQz[i] - baseFrame->mBase; break;
                break;
            }
        }

        assert(componentIndexPtr - ad->mComponentIndices == ad->mNumAnimatedComponents);

        CHOMP("}");
        CHOMP("bounds");
        CHOMP("{");

        BBox *BBoxes = ad->mBoundingBoxes;
        for (int i = 0, e = numFrames; i < e; ++i)
        {
            CHOMP("(");
            ParsedFloat minX = parser.ParseFloat();
            ParsedFloat minY = parser.ParseFloat();
            ParsedFloat minZ = parser.ParseFloat();
            CHOMP(")");
            CHOMP("(");
            ParsedFloat maxX = parser.ParseFloat();
            ParsedFloat maxY = parser.ParseFloat();
            ParsedFloat maxZ = parser.ParseFloat();
            CHOMP(")");

            if (!minX.Valid()
                || !minY.Valid()
                || !minZ.Valid()
                || !maxX.Valid()
                || !maxY.Valid()
                || !maxZ.Valid())
                return NULL;

            BBoxes[i] = BBox(Vec3(minX, minY, minZ), Vec3(maxX, maxY, maxZ));
        }

        CHOMP("}");

        CHOMP("baseframe");
        CHOMP("{");

        SoaQuatPos &qp = ad->mBaseFrame;
        {
            float *x = qp.mX, *y = qp.mY, *z = qp.mZ, *qqx = qp.mQx, *qqy = qp.mQy, *qqz = qp.mQz;

            for (int i = 0, e = numJoints; i < e; ++i)
            {
                CHOMP("(");
                // position
                ParsedFloat px = parser.ParseFloat();
                ParsedFloat py = parser.ParseFloat();
                ParsedFloat pz = parser.ParseFloat();
                CHOMP(")");

                CHOMP("(");
                // rotation
                ParsedFloat qx = parser.ParseFloat();
                ParsedFloat qy = parser.ParseFloat();
                ParsedFloat qz = parser.ParseFloat();
                CHOMP(")");

                if (!px.Valid()
                    || !py.Valid()
                    || !pz.Valid()
                    || !qx.Valid()
                    || !qy.Valid()
                    || !qz.Valid())
                    return NULL;

                x[i] = px;
                y[i] = py;
                z[i] = pz;
                qqx[i] = qx;
                qqy[i] = qy;
                qqz[i] = qz;
            }

            qp.UncompressQw();
        }

        CHOMP("}");

        float *componentFrames = ad->mComponentFrames;
        for (int i = 0, e = numFrames; i < e; ++i)
        {
            CHOMP("frame");
            ParsedInt frame = parser.ParseInt();

            if (frame != i)
                return NULL;

            CHOMP("{");
            for (int ii = 0, ee = numAnimatedComponents; ii < ee; ++ii)
            {
                ParsedFloat component = parser.ParseFloat();
                if (!component.Valid())
                    return NULL;
                *componentFrames++ = component;
            }
            CHOMP("}");
        }

        return ad;
    }

    void InterpolateAnimationFrames(SoaQuatPos *interpolated, const AnimData *animData, int frameOneIdx, int frameTwoIdx, float lerp)
    {
        assert(frameOneIdx < animData->mNumFrames);
        assert(frameTwoIdx < animData->mNumFrames);

        const SoaQuatPos *baseFrame = &animData->mBaseFrame;
        const int numElements = baseFrame->mNumElements;
        const size_t size = SoaQuatPos::MemorySize(numElements);
        float * const frameOneBase = static_cast<float *>(AllocaAligned(SIMD_ALIGNMENT, size));
        float * const frameTwoBase = static_cast<float *>(AllocaAligned(SIMD_ALIGNMENT, size));

        SoaQuatPos frameOne(numElements, frameOneBase);
        SoaQuatPos frameTwo(numElements, frameTwoBase);
        float * const base = baseFrame->mBase;

        memcpy(frameOne.mBase, base, size);
        memcpy(frameTwo.mBase, base, size);

        const int numComponents = animData->mNumAnimatedComponents;
        float * const frameOneComponents = animData->mComponentFrames + numComponents * frameOneIdx;
        float * const frameTwoComponents = animData->mComponentFrames + numComponents * frameTwoIdx;
        int * indices = animData->mComponentIndices;

        for (int i = 0; i < numComponents; ++i)
        {
            const int index = indices[i];
            const float f1 = frameOneComponents[i];
            const float f2 = frameTwoComponents[i];
            frameOneBase[index] = f1;
            frameTwoBase[index] = f2;
        }

        interpolated->Interpolate(&frameOne, &frameTwo, lerp);
    }
}

