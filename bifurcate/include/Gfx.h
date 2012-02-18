#ifndef BIFURCATE_GFX_H
#define BIFURCATE_GFX_H

#include <MathTypes.h>

namespace bg
{
    struct draw_element_buffer;
    struct vertex
    {
        vec3 position;
        vec2 uv;
        vec3 normal;
    };

    draw_element_buffer *deb_create(int numVertices, int numIndices);    
    void deb_set_vertices(draw_element_buffer *buffer, bg::vertex *vertices);
    void deb_set_indices(draw_element_buffer *buffer, unsigned short *indices);
    void deb_destroy(draw_element_buffer *buffer);

    int gfx_initialize(void *instance, int width, int height);
    void gfx_shutdown();
    void gfx_begin_scene();
    void gfx_end_scene();
}

#endif
