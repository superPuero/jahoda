#ifndef jahoda_gpu_texture
#define jahoda_gpu_texture

#include <jahoda/core/core.h>
#include <jahoda/gfx/gpu_glfw.h>

typedef enum
{
    gpu_texture_state_undefined = 0,
    gpu_texture_state_color_attachment,    
    gpu_texture_state_depth_attachment,    
    gpu_texture_state_depth_read_only,     
    gpu_texture_state_vertex_read,   
    gpu_texture_state_fragment_read,
    gpu_texture_state_present,
} gpu_texture_state;

typedef struct
{
    text128 tag;

    VkImage handle;
    VkImageView view_handle;
    VkFormat format;
    gpu_texture_state state;
    VkExtent3D extent;

    void *mem;
} gpu_texture;

typedef gpu_texture gpu_pfif_texture[frames_in_flight];

#endif