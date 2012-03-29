#include "Mesh.h"
#include "Core.h"
#include "Parser.h"

namespace bg
{
    static void BuildBindPoses(SkinnedMeshData *md)
    {
        int numJoints = md->mNumJoints;
        Mat4x4 *bindPoseArray = static_cast<Mat4x4 *>(MemAlloc(bc::POOL_MESH, sizeof(Mat4x4) * numJoints));
        Mat4x4 *invBindPoseArray = static_cast<Mat4x4 *>(MemAlloc(bc::POOL_MESH, sizeof(Mat4x4) * numJoints));
        Joint *joints = md->mJoints;

        for (int i = 0; i < numJoints; ++i)
        {
            Mat4x4FromQuatPos(bindPoseArray + i, &joints[i].mInitial);
            Mat4x4Invert(invBindPoseArray + i, bindPoseArray + i);
        }

        md->mBindPose = bindPoseArray;
        md->mInverseBindPose = invBindPoseArray;
    }

    const SkinnedMeshData *LoadMesh(const char *fileName)
    {
        using namespace bc;
        using namespace bx;

        #define CHOMP(string) if (!parser.ExpectAndDiscard(string)) SignalErrorAndReturn(NULL, "Unable to chomp string '%s'.", string); else (void)0
        #define PARSE_INT(value) CHOMP(#value); ParsedInt value = parser.ParseInt(); if (!value.Valid()) SignalErrorAndReturn(NULL, "Unable to parse integer for value '%s'.", #value)

        AutoMemMap file(fileName);
        if (!file.Valid())
            SignalErrorAndReturn(NULL, "Invalid file '%s'.", fileName);

        Parser parser(static_cast<const char *>(file.Mem()), static_cast<const char *>(file.Mem()) + file.Size());
        CHOMP("MD5Version");
        ParsedInt version = parser.ParseInt();
        if (version != 10)
            SignalErrorAndReturn(NULL, "Invalid md5mesh version %d.", version.Value());

        CHOMP("commandline");
        /* ParsedString commandLine = */ parser.ParseString();

        SkinnedMeshData *md = static_cast<SkinnedMeshData *>(MemAlloc(POOL_MESH, sizeof(SkinnedMeshData)));
        CHOMP("numJoints");
        ParsedInt numJoints = parser.ParseInt();
        CHOMP("numMeshes");
        ParsedInt numMeshes = parser.ParseInt();

        if (!numJoints.Valid()
            || !numMeshes.Valid())
            SignalErrorAndReturn(NULL, "Unable to parse number of joints and/or number of meshes.");

        md->mNumJoints = numJoints;
        md->mNumMeshes = numMeshes;

        CHOMP("joints");
        CHOMP("{");

        md->mJoints = static_cast<Joint *>(MemAlloc(POOL_MESH, sizeof(Joint) * md->mNumJoints));
        md->mIndexBuffers = static_cast<IndexBuffer **>(MemAlloc(POOL_MESH, sizeof(IndexBuffer *) * md->mNumMeshes));
        md->mVertexBuffers = static_cast<VertexBuffer **>(MemAlloc(POOL_MESH, sizeof(VertexBuffer *) * md->mNumMeshes));
        md->mWeightedPositionBuffers = static_cast<MeshWeightedPositionBuffer **>(MemAlloc(POOL_MESH, sizeof(MeshWeightedPositionBuffer *) * md->mNumMeshes));
        md->mNumTris = static_cast<int *>(MemAlloc(POOL_MESH, sizeof(int) * md->mNumMeshes));

        Joint *joints = md->mJoints;
        for (int i = 0, e = numJoints; i < e; ++i)
        {
            ParsedString jointName = parser.ParseString();
            ParsedInt parentIndex = parser.ParseInt();

            CHOMP("(");
            ParsedFloat px = parser.ParseFloat();
            ParsedFloat py = parser.ParseFloat();
            ParsedFloat pz = parser.ParseFloat();
            CHOMP(")");

            CHOMP("(");
            ParsedFloat qx = parser.ParseFloat();
            ParsedFloat qy = parser.ParseFloat();
            ParsedFloat qz = parser.ParseFloat();
            CHOMP(")");

            if (!parentIndex.Valid()
                || !px.Valid() || !py.Valid() || !pz.Valid()
                || !qx.Valid() || !qy.Valid() || !qz.Valid())
                SignalErrorAndReturn(NULL, "Unable to parse joint information for joint %d.", i);

            if (!Intern(&joints[i].mName, &joints[i].mNameHash, jointName.mBegin, jointName.mEnd))
                SignalErrorAndReturn(NULL, "Unable to intern joint name.");
 
            joints[i].mInitial = QuatPos(px, py, pz, qx, qy, qz);
        }

        CHOMP("}");

        for (int i = 0, e = numMeshes; i < e; ++i)
        {
            TempMemMarker marker;

            CHOMP("mesh");
            CHOMP("{");

            CHOMP("shader");
            /* ParsedString material = */ parser.ParseString();

            PARSE_INT(numverts);
            MeshVertex *vertices = static_cast<MeshVertex *>(TempAlloc(sizeof(MeshVertex) * numverts));
            for (int ii = 0, ee = numverts; ii < ee; ++ii)
            {
                CHOMP("vert");
                /* ParsedInt vertexIndex = */ parser.ParseInt();
                CHOMP("(");
                ParsedFloat u = parser.ParseFloat();
                ParsedFloat v = parser.ParseFloat();
                CHOMP(")");
                ParsedInt weightIndex = parser.ParseInt();
                ParsedInt weightElem = parser.ParseInt();

                if (!u.Valid() || !v.Valid() || !weightIndex.Valid() || !weightElem.Valid())
                    SignalErrorAndReturn(NULL, "Mesh u/v/weights not valid for vertex %d of mesh %d.", ii, i);

                vertices[ii].mTexU = u;
                vertices[ii].mTexV = v;
                vertices[ii].mWeightIndex = static_cast<short>(weightIndex);
                vertices[ii].mWeightElement = static_cast<short>(weightElem);
            }
            md->mVertexBuffers[i] = VertexBufferCreate(numverts, sizeof(MeshVertex), vertices);
            marker.Reset();

            PARSE_INT(numtris);
            md->mNumTris[i] = numtris;
            unsigned short *indices = static_cast<unsigned short *>(TempAlloc(sizeof(unsigned short) * numtris));
            for (int ii = 0, writeIndex = 0, ee = numtris; ii < ee; ++ii)
            {
                CHOMP("tri");
                /* ParsedInt triIndex = */ parser.ParseInt();
                ParsedInt i0 = parser.ParseInt();
                ParsedInt i1 = parser.ParseInt();
                ParsedInt i2 = parser.ParseInt();

                if (!i0.Valid() || !i1.Valid() || !i2.Valid())
                    SignalErrorAndReturn(NULL, "Triangle indices not valid for tri %d of mesh %d.", ii, i);

                assert(i0 <= 65535 && i1 <= 65535 && i2 <= 65535);

                indices[writeIndex++] = static_cast<unsigned short>(i0);
                indices[writeIndex++] = static_cast<unsigned short>(i1);
                indices[writeIndex++] = static_cast<unsigned short>(i2);
            }
            md->mIndexBuffers[i] = IndexBufferCreate(numtris * 3, indices);
            marker.Reset();

            PARSE_INT(numweights);
            Vec4 *weightedPositions = static_cast<Vec4 *>(TempAlloc(sizeof(Vec4) * numweights));
            unsigned char *jointIndices = static_cast<unsigned char *>(TempAlloc(sizeof(unsigned char) * numweights));

            for (int ii = 0, ee = numweights; ii < ee; ++ii)
            {
                CHOMP("weight");
                /* ParsedInt weightIndex = */ parser.ParseInt();
                ParsedInt jointIndex = parser.ParseInt();
                ParsedFloat weight = parser.ParseFloat();
                CHOMP("(");
                ParsedFloat x = parser.ParseFloat();
                ParsedFloat y = parser.ParseFloat();
                ParsedFloat z = parser.ParseFloat();
                CHOMP(")");

                if (!jointIndex.Valid() 
                    || !weight.Valid()
                    || !x.Valid()
                    || !y.Valid()
                    || !z.Valid())
                    SignalErrorAndReturn(NULL, "Unable to parse weight information at index %d for mesh %d.", ii, i);

                if (jointIndex >= bg::MAX_NUM_JOINTS)
                    SignalErrorAndReturn(NULL, "Too many joints for model, max number of supported joints is %d.", bg::MAX_NUM_JOINTS);

                Vec4 weightedPosition = { x, y, z, weight };
                weightedPositions[ii] = weightedPosition;
                jointIndices[ii] = static_cast<unsigned char>(jointIndex);
            }
            md->mWeightedPositionBuffers[i] = MeshWeightedPositionBufferCreate(numweights, weightedPositions, jointIndices);

            CHOMP("}");
        }

        BuildBindPoses(md);

        return md;
    }
}
