#ifndef BIFURCATE_MESH_H
#define BIFURCATE_MESH_H

namespace bg
{
    struct joint
    {
        const char *mName;
        uint64_t mNameHash;
        quat_pos mInitial;
    };

    struct mesh
    {
    };

    struct mesh_data
    {
        int mNumFrames;
        int mNumMeshes;
        joint *mJoints;
    };

    const mesh_data *load_mesh(const char *file_name);
}

#endif
