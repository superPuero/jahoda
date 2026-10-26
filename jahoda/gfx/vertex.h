#ifndef jahoda_vertex
#define jahoda_vertex

#include <jahoda/core/math.h>
#include "gpu_glfw.h"

#define render_vertex_attribute_list\
    X(position, vec3_f32, VK_FORMAT_R32G32B32_SFLOAT)\
    X(normal, vec3_f32, VK_FORMAT_R32G32B32_SFLOAT)\
    X(uv, vec2_f32, VK_FORMAT_R32G32_SFLOAT)\
    X(tangent, vec4_f32, VK_FORMAT_R32G32B32A32_SFLOAT)\
    
static uz render_vertex_size = (
    #define X(name, type, format) sizeof(type) +
    render_vertex_attribute_list
    #undef X
0);

typedef enum
{
    #define X(name, type, vk_type) render_vertex_attribute_##name,
    render_vertex_attribute_list
    #undef X
} render_vertex_attribute;

typedef enum
{
    #define X(name, type, vk_type) render_vertex_attribute_bit_##name = bit(render_vertex_attribute_##name),
    render_vertex_attribute_list
    #undef X
} render_vertex_attribute_bit;

static VkFormat render_vertex_attribute_format[] = {
    #define X(name, type, format) [render_vertex_attribute_##name] = format,
    render_vertex_attribute_list
    #undef X
};

static VkFormat render_vertex_attribute_size[] = {
    #define X(name, type, format) [render_vertex_attribute_##name] = sizeof(type),
    render_vertex_attribute_list
    #undef X
};


#endif