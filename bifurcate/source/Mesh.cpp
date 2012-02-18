#include "Mesh.h"
#include "Core.h"

namespace bg
{
    const mesh_data *load_mesh(const char *file_name)
    {
        using namespace bc;

        #define CHOMP(string) if (!parser.expect_and_discard(string)) return NULL; else (void)0

        auto_mem_map(file_name);
        if (!file.valid())
            return NULL;

        parser parser(static_cast<const char *>(file.mem()), static_cast<const char *>(file.mem()) + file.size());
        CHOMP("MD5Version");
        if (parser.parse_int() != 10)
            return NULL;
        CHOMP("commandline");
        /* parsed_string commandLine = */ parser.parse_string();

        mesh_data *md = static_cast<mesh_data>(mem_alloc(POOL_MESH, sizeof(mesh_data)));
        CHOMP("numJoints");
        parsed_int numJoints = parser.parse_int();
        CHOMP("numMeshes");
        parsed_int numMeshes = parser.parse_int();

        if (!numFrames.valid()
            || !numMeshes.valid())
            return NULL;

        md->mNumJoints = numJoints;
        md->mNumMeshes = numMeshes;

        CHOMP("joints");
        CHOMP("{");

        md->mJoints = static_cast<joint *>(mem_alloc(POOL_MESH, sizeof(joint) * md->mNumJoints));

        joint *joints = md->mJoints;
        for (int i = 0; i < numJoints; ++i)
        {
            parsed_string jointName = parser.parse_string();
            parsed_int parentIndex = parser.parse_int();

            CHOMP("(");
            parsed_float px = parser.parse_float();
            parsed_float py = parser.parse_float();
            parsed_float pz = parser.parse_float();
            CHOMP(")");

            CHOMP("(");
            parsed_float qx = parser.parse_float();
            parsed_float qy = parser.parse_float();
            parsed_float qz = parser.parse_float();
            CHOMP(")");

            if (!parentIndex.valid()
                || !px.valid() || !py.valid() || !pz.valid()
                || !qx.valid() || !qy.valid() || !qz.valid())
                return NULL;

            joints[i].mInitial = quat_pos(px, py, pz, qx, qy, qz);
        }
    }
}
