#ifndef jahoda_vk_device
#define jahoda_vk_device

#include "vk_glfw.h"
#include "vk_instance.h"
#include "vk_physical_device.h"
#include "vk_surface.h"

#define jahoda_vk_invalid_queue_family (u32)(-1)

da_declare(VkQueueFamilyProperties, vk_queue_family_properties_da);
da_declare(VkDeviceQueueCreateInfo, vk_device_queue_ci_da);
da_declare(VkExtensionProperties, vk_exntension_propertioes_da);

typedef struct
{
	VkDevice handle;
	
	u32 queue_graphics_index;
	u32 queue_transfer_index;
	u32 queue_compute_index;
	u32 queue_present_index;

}vk_device;

vk_device vk_device_make(arena *temp_memory, vk_physical_device *pdevice, vk_surface *surface);
void vk_device_release(vk_device *device);

#endif