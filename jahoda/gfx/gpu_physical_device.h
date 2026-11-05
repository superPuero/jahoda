#ifndef jahoda_gpu_physical_device
#define jahoda_gpu_physical_device

#include "gpu_glfw.h"
#include "gpu_instance.h"

da_declare(VkFormat, gpu_format_da);

typedef struct
{
	VkPhysicalDevice handle;
	VkPhysicalDeviceProperties properties;    
	VkPhysicalDeviceProperties2 properties2;
	VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptor_buffer_properties;
	gpu_format_da supported_depth_formats;
}gpu_physical_device;

gpu_physical_device gpu_physical_device_make(arena *temp_arena, arena *static_arena, gpu_instance *instance);
u32 gpu_physical_device_pick_memory_type(gpu_physical_device *pdevice, u32 type_filter, VkMemoryPropertyFlags properties);

#endif