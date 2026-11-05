#include "gpu_physical_device.h"
#include "vulkan/vulkan_core.h"

gpu_physical_device gpu_physical_device_make(arena *temp_arena, arena *static_arena, gpu_instance *instance)
{
	gpu_physical_device out = {0};

	u32 physical_device_count = 0;
	vkEnumeratePhysicalDevices(instance->handle, &physical_device_count, NULL);

	da_declare(VkPhysicalDevice, gpu_physical_device_da);

	gpu_physical_device_da physical_devices = {0};
	da_resize(&physical_devices, temp_arena, physical_device_count);
	gpu_check(vkEnumeratePhysicalDevices(instance->handle, &physical_device_count, physical_devices.data));

	// @todo: Physical device selection mechanism
	out.handle = physical_devices.data[0];

	da_declare(VkFormat, gpu_format_device_da);

	gpu_format_device_da candidates = {0};

	da_append(&candidates, temp_arena, VK_FORMAT_D32_SFLOAT);
	da_append(&candidates, temp_arena, VK_FORMAT_D32_SFLOAT_S8_UINT);
	da_append(&candidates, temp_arena, VK_FORMAT_D24_UNORM_S8_UINT);

	vkGetPhysicalDeviceProperties(out.handle, &out.properties);

	out.descriptor_buffer_properties = (VkPhysicalDeviceDescriptorBufferPropertiesEXT){
    	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
	};

	out.properties2 = (VkPhysicalDeviceProperties2){
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &out.descriptor_buffer_properties
	};
	vkGetPhysicalDeviceProperties2(out.handle, &out.properties2);
	

	for da_each(it, &candidates)
	{
		VkFormatProperties props;
		VkFormat format = *it;
		vkGetPhysicalDeviceFormatProperties(out.handle, format, &props);
	
		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			da_append(&out.supported_depth_formats, static_arena, format);
		}
	}

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(out.handle, &features);

	if (features.sparseBinding && features.sparseResidencyBuffer) 
	{
		
	}



	return out;
}

u32 gpu_physical_device_pick_memory_type(gpu_physical_device *pdevice, u32 type_filter, VkMemoryPropertyFlags properties)
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