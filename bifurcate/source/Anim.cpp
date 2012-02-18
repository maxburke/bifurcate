#include <string.h>

#include "Anim.h"
#include "Parser.h"
#include "Core.h"

namespace bg
{
    const anim_data *load_anim(const char *file_name)
    {
        using namespace bc;
        using namespace bx;

        #define CHOMP(string) if (!parser.expect_and_discard(string)) return NULL; else (void)0

        auto_mem_map file(file_name);

        if (!file.valid())
            return NULL;

        parser parser(static_cast<const char *>(file.mem()), static_cast<const char *>(file.mem()) + file.size());
        CHOMP("MD5Version");
        if (parser.parse_int() != 10) return NULL;
        CHOMP("commandline");
        /* parsed_string commandLine = */ parser.parse_string();

        anim_data *ad = static_cast<anim_data *>(mem_alloc(POOL_ANIM, sizeof(anim_data)));

        CHOMP("numFrames");
        parsed_int numFrames = parser.parse_int();
        CHOMP("numJoints");
        parsed_int numJoints = parser.parse_int();
        CHOMP("frameRate");
        parsed_int frameRate = parser.parse_int();
        CHOMP("numAnimatedComponents");
        parsed_int numAnimatedComponents = parser.parse_int();

        if (!numFrames.valid()
            || !numJoints.valid()
            || !frameRate.valid()
            || !numAnimatedComponents.valid())
            return NULL;

        ad->mNumFrames = numFrames;
        ad->mNumJoints = numJoints;
        ad->mFrameRate = frameRate;
        ad->mNumAnimatedComponents = numAnimatedComponents;
        ad->mJoints = static_cast<anim_joint *>(mem_alloc(POOL_ANIM, sizeof(anim_joint) * ad->mNumJoints));
        ad->mBoundingBoxes = static_cast<bbox *>(mem_alloc(POOL_ANIM, sizeof(bbox) * ad->mNumFrames));
        ad->mBaseFrame = static_cast<quat_pos *>(mem_alloc(POOL_ANIM, sizeof(quat_pos) * ad->mNumJoints));
        ad->mComponentFrames = static_cast<float *>(mem_alloc(POOL_ANIM, sizeof(float) * ad->mNumAnimatedComponents * ad->mNumFrames));

        CHOMP("hierarchy");
        CHOMP("{");

        anim_joint *joints = ad->mJoints;
        for (int i = 0; i < numJoints; ++i)
        {
            parsed_string jointName = parser.parse_string();
            parsed_int parentIndex = parser.parse_int();
            parsed_int jointFlags = parser.parse_int();
            parsed_int firstComponent = parser.parse_int();

            if (!jointName.valid()
                || !parentIndex.valid()
                || !jointFlags.valid()
                || !firstComponent.valid())
                return NULL;

            const size_t nameLength = jointName.length();
            char *name = static_cast<char *>(mem_alloc(POOL_STRING, jointName.length() + 1));
            memcpy(name, jointName.mBegin, nameLength);
            name[nameLength] = 0;

            joints[i].mName = name;
            joints[i].mNameHash = bc::hash(jointName.mBegin, jointName.mEnd);
            joints[i].mJointFlags = jointFlags;
            joints[i].mParentIndex = parentIndex;
            joints[i].mFirstComponent = firstComponent;
        }

        CHOMP("}");
        CHOMP("bounds");
        CHOMP("{");

        bbox *bboxes = ad->mBoundingBoxes;
        for (int i = 0; i < numFrames; ++i)
        {
            CHOMP("(");
            parsed_float minX = parser.parse_float();
            parsed_float minY = parser.parse_float();
            parsed_float minZ = parser.parse_float();
            CHOMP(")");
            CHOMP("(");
            parsed_float maxX = parser.parse_float();
            parsed_float maxY = parser.parse_float();
            parsed_float maxZ = parser.parse_float();
            CHOMP(")");

            if (!minX.valid()
                || !minY.valid()
                || !minZ.valid()
                || !maxX.valid()
                || !maxY.valid()
                || !maxZ.valid())
                return NULL;

            bboxes[i] = bbox(vec3(minX, minY, minZ), vec3(maxX, maxY, maxZ));
        }

        CHOMP("}");

        CHOMP("baseframe");
        CHOMP("{");

        quat_pos *qp = ad->mBaseFrame;
        for (int i = 0; i < numJoints; ++i)
        {
            CHOMP("(");
            // position
            parsed_float px = parser.parse_float();
            parsed_float py = parser.parse_float();
            parsed_float pz = parser.parse_float();
            CHOMP(")");

            CHOMP("(");
            // rotation
            parsed_float qx = parser.parse_float();
            parsed_float qy = parser.parse_float();
            parsed_float qz = parser.parse_float();
            CHOMP(")");

            if (!px.valid()
                || !py.valid()
                || !pz.valid()
                || !qx.valid()
                || !qy.valid()
                || !qz.valid())
                return NULL;

            qp[i] = quat_pos(px, py, pz, qx, qy, qz);
        }

        CHOMP("}");

        float *componentFrames = ad->mComponentFrames;
        for (int i = 0; i < numFrames; ++i)
        {
            CHOMP("frame");
            parsed_int frame = parser.parse_int();

            if (frame != i)
                return NULL;

            CHOMP("{");
            for (int ii = 0; ii < numAnimatedComponents; ++ii)
            {
                parsed_float component = parser.parse_float();
                if (!component.valid())
                    return NULL;
                *componentFrames++ = component;
            }
            CHOMP("}");
        }

        return ad;
    }
}

