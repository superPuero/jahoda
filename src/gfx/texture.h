#ifndef jahoda_texture
#define jahoda_texture

#include "vk_glfw.h"
#include "gpu_arena.h"

typedef struct
{
	VkImage image;
    VkImageView view;
} texture;

void texture_release(vk_device* device, texture* texture);

#endif