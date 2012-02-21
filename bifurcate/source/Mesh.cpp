#include "Mesh.h"
#include "Core.h"
#include "Parser.h"

namespace bg
{
    const MeshData *LoadMesh(const char *fileName)
    {
        using namespace bc;
        using namespace bx;

        #define CHOMP(string) if (!parser.ExpectAndDiscard(string)) return NULL; else (void)0
        #define PARSE_INT(value) CHOMP(#value); ParsedInt value = parser.ParseInt(); if (!value.Valid()) return NULL

        AutoMemMap file(fileName);
        if (!file.Valid())
            return NULL;

        Parser parser(static_cast<const char *>(file.Mem()), static_cast<const char *>(file.Mem()) + file.Size());
        CHOMP("MD5Version");
        if (parser.ParseInt() != 10)
            return NULL;
        CHOMP("commandline");
        /* ParsedString commandLine = */ parser.ParseString();

        MeshData *md = static_cast<MeshData *>(MemAlloc(POOL_MESH, sizeof(MeshData)));
        CHOMP("numJoints");
        ParsedInt numJoints = parser.ParseInt();
        CHOMP("numMeshes");
        ParsedInt numMeshes = parser.ParseInt();

        if (!numJoints.Valid()
            || !numMeshes.Valid())
            return NULL;

        md->mNumJoints = numJoints;
        md->mNumMeshes = numMeshes;

        CHOMP("joints");
        CHOMP("{");

        md->mJoints = static_cast<Joint *>(MemAlloc(POOL_MESH, sizeof(Joint) * md->mNumJoints));
        md->mIndexBuffers = static_cast<IndexBuffer **>(MemAlloc(POOL_MESH, sizeof(IndexBuffer *) * md->mNumMeshes));
        md->mVertexBuffers = static_cast<MeshVertexBuffer **>(MemAlloc(POOL_MESH, sizeof(MeshVertexBuffer *) * md->mNumMeshes));
        md->mWeightedPositionBuffers = static_cast<MeshWeightedPositionBuffer **>(MemAlloc(POOL_MESH, sizeof(MeshWeightedPositionBuffer *) * md->mNumMeshes));

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
                return NULL;

            if (!Intern(&joints[i].mName, &joints[i].mNameHash, jointName.mBegin, jointName.mEnd))
                return NULL;
 
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
                    return NULL;

                vertices[i].mTexCoords.x = u;
                vertices[i].mTexCoords.y = v;
                vertices[i].mWeightIndex = static_cast<short>(weightIndex);
                vertices[i].mWeightElement = static_cast<short>(weightElem);
            }
            md->mVertexBuffers[i] = MeshVertexBufferCreate(numverts, vertices);
            marker.Reset();

            PARSE_INT(numtris);
            unsigned short *indices = static_cast<unsigned short *>(TempAlloc(sizeof(unsigned short) * numtris));
            for (int ii = 0, writeIndex = 0, ee = numtris; ii < ee; ++ii)
            {
                CHOMP("tri");
                /* ParsedInt triIndex = */ parser.ParseInt();
                ParsedInt i0 = parser.ParseInt();
                ParsedInt i1 = parser.ParseInt();
                ParsedInt i2 = parser.ParseInt();

                if (!i0.Valid() || i1.Valid() || i2.Valid())
                    return NULL;

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
                    return NULL;

                weightedPositions[ii] = Vec4(x, y, z, weight);
                jointIndices[ii] = static_cast<unsigned char>(jointIndex);
            }
            md->mWeightedPositionBuffers[i] = MeshWeightedPositionBufferCreate(numweights, weightedPositions, jointIndices);
        }

        return md;
    }
}
