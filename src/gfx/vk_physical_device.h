#ifndef jahoda_vk_physical_device
#define jahoda_vk_physical_device

#include "vk_glfw.h"
#include "vk_instance.h"

da_declare(VkFormat, vk_format_da);

typedef struct
{
	VkPhysicalDevice handle;
	vk_format_da supported_depth_formats;
}vk_physical_device;

vk_physical_device vk_physical_device_make(arena* temp_arena, arena* static_arena, vk_instance* instance);
u32 vk_physical_device_pick_memory_type(vk_physical_device* pdevice, u32 type_filter, VkMemoryPropertyFlags properties);

#endif