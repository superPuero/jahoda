#include "vk_physical_device.h"

vk_physical_device vk_physical_device_make(arena* temp_arena, arena* static_arena, vk_instance* instance)
{
	vk_physical_device out = {0};

	u32 physical_device_count = 0;
	vkEnumeratePhysicalDevices(instance->handle, &physical_device_count, NULL);

	da_declare(VkPhysicalDevice, vk_physical_device_da);

	vk_physical_device_da physical_devices = {0};
	da_resize(temp_arena, &physical_devices, physical_device_count);
	vk_check(vkEnumeratePhysicalDevices(instance->handle, &physical_device_count, physical_devices.data));

	// @todo: Physical device selection mechanism
	out.handle = physical_devices.data[0];

	da_declare(VkFormat, vk_format_device_da);

	vk_format_device_da candidates = {0};

	da_append(temp_arena, &candidates, VK_FORMAT_D32_SFLOAT);
	da_append(temp_arena, &candidates, VK_FORMAT_D32_SFLOAT_S8_UINT);
	da_append(temp_arena, &candidates, VK_FORMAT_D24_UNORM_S8_UINT);

	da_foreach(&candidates)
	{
		VkFormatProperties props;
		VkFormat format = *candidates.it;
		vkGetPhysicalDeviceFormatProperties(out.handle, format, &props);

		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			da_append(static_arena, &out.supported_depth_formats, format);
		}
	}

	return out;
}

u32 vk_physical_device_pick_memory_type(vk_physical_device* pdevice, u32 type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_props = {0};
	vkGetPhysicalDeviceMemoryProperties(pdevice->handle, &mem_props);

	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
	{
		if ((type_filter & (1 << i)) &&
			(mem_props.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	err("none of the provided memory types have requested memory properies");
}