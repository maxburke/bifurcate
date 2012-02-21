#ifndef BIFURCATE_MAP_H
#define BIFURCATE_MAP_H
#if 0
#include "Decls.h"

namespace bg
{
    struct draw_element_buffer;

    struct surface
    {
        const char *material;
        int material_index;
        int num_triangles;
        draw_element_buffer *element_buffer;
    };

    #pragma warning(push)
    #pragma warning(disable:4200)
    struct model
    {
        const char *name;
        int is_area;
        surface surfaces[0];
    };
    #pragma warning(pop)

    #pragma warning(push)
    #pragma warning(disable:4200)
    struct portal
    {
        int num_points;
        int positive_side_area;
        int negative_side_area;
        Vec3 points[0];
    };
    #pragma warning(pop)

    struct node
    {
        int Plane_index;
        int positive_child;
        int negative_child;
    };

    struct shadow_model
    {
        const char *name;
        int no_caps;
        int no_front_caps;
        int num_primitives;
        int Plane_bits;
        draw_element_buffer *element_buffer;
    };
    /*
    struct MapData
    {
        unsigned mSignature;
        unsigned mModelsOffset;
        unsigned mSurfacesOffset;
        unsigned mPortalsOffset;
        unsigned mNodesOffset;
        unsigned mShadowModelOffset;
        unsigned mSurfaceVerticesOffset;
        unsigned mSurfaceIndicesOffset;
        unsigned mbg_PlanesOffset;
        unsigned mNamesOffset;
        unsigned mShadowVerticesOffset;
        unsigned mShadowIndicesOffset;
    };
    */
}
#endif
#endif