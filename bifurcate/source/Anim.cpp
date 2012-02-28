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

        CHOMP("hierarchy");
        CHOMP("{");

        AnimJoint *joints = ad->mJoints;
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
        }

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
}

