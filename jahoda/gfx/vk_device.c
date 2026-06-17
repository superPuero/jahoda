#include "vk_device.h"

void validate_device_extensions(arena *temp_arena, vk_physical_device *pdevice, cstr_da *device_extensions)
{
	u32 device_extension_count;
	vkEnumerateDeviceExtensionProperties(pdevice->handle, NULL, &device_extension_count, NULL);

	vk_exntension_propertioes_da avalible_device_extensions = {0};
	da_resize(temp_arena, &avalible_device_extensions, device_extension_count);
	vkEnumerateDeviceExtensionProperties(pdevice->handle, NULL, &device_extension_count, avalible_device_extensions.data);

	for da_each(device_extensions, device_extensions_it)
	{
		
		bool8 found = false;
		for da_each(&avalible_device_extensions, avalible_device_extensions_id)
		{
			if (strcmp(*device_extensions_it, avalible_device_extensions_id->extensionName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			err("required device extension '%s' is not supported", *device_extensions_it);
		}
	}	
}

vk_device vk_device_make(arena *temp_arena, vk_physical_device *pdevice, vk_surface *surface)
{
	vk_device out = {0};
	out.queue_graphics_index = jahoda_vk_invalid_queue_family;
	out.queue_transfer_index = jahoda_vk_invalid_queue_family;
	out.queue_compute_index = jahoda_vk_invalid_queue_family;
	out.queue_present_index = jahoda_vk_invalid_queue_family;

	u32 queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pdevice->handle, &queue_families_count, NULL);

	vk_queue_family_properties_da queue_families = {0};
	da_resize(temp_arena, &queue_families, queue_families_count);

	vkGetPhysicalDeviceQueueFamilyProperties(pdevice->handle, &queue_families_count, queue_families.data);
		
	for (u32 i = 0; i < queue_families.occupied; i++)
	{
		VkQueueFamilyProperties family = queue_families.data[i];

		if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT
			&& out.queue_graphics_index == jahoda_vk_invalid_queue_family)
		{
			out.queue_graphics_index = i;
		}

		if (family.queueFlags & VK_QUEUE_TRANSFER_BIT
			&& out.queue_graphics_index != i
			&& out.queue_transfer_index == jahoda_vk_invalid_queue_family)
		{
			out.queue_transfer_index = i;
		}

		if (family.queueFlags & VK_QUEUE_COMPUTE_BIT
			&& out.queue_graphics_index != i
			&& out.queue_transfer_index != i
			&& out.queue_compute_index == jahoda_vk_invalid_queue_family)
		{
			out.queue_compute_index = i;
		}

		VkBool32 present_supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(pdevice->handle, i, surface->handle, &present_supported);

		if (present_supported
			&& out.queue_compute_index != i
			&& out.queue_graphics_index != i
			&& out.queue_transfer_index != i
			&& out.queue_present_index == jahoda_vk_invalid_queue_family
			)
		{
			out.queue_present_index = i;
		}

		if (out.queue_graphics_index != jahoda_vk_invalid_queue_family
			&& out.queue_transfer_index != jahoda_vk_invalid_queue_family
			&& out.queue_compute_index != jahoda_vk_invalid_queue_family
			&& out.queue_present_index != jahoda_vk_invalid_queue_family)
		{
			break;
		}
	}

	if (out.queue_compute_index == jahoda_vk_invalid_queue_family)
	{
		infol(vulkan_device, "wasnt able to find dedicated compute queue");

		for (uz i = 0; i < queue_families.occupied; i++)
		{
			VkQueueFamilyProperties family = queue_families.data[i];

			if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				out.queue_compute_index = i;
				break;
			}
		}
	}

	if (out.queue_present_index == jahoda_vk_invalid_queue_family)
	{
		info("wasnt able to find dedicated present queue");

		for (uz i = 0; i < queue_families.occupied; i++)
		{
			VkBool32 present_supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(pdevice->handle, i, surface->handle, &present_supported);

			if (present_supported)
			{
				out.queue_present_index = i;
				break;
			}
		}
	}

	dbg_verify(out.queue_graphics_index != jahoda_vk_invalid_queue_family, "");
	dbg_verify(out.queue_transfer_index != jahoda_vk_invalid_queue_family, "");
	dbg_verify(out.queue_compute_index != jahoda_vk_invalid_queue_family, "");
	dbg_verify(out.queue_present_index != jahoda_vk_invalid_queue_family, "");

	infol(vulkan_device, "out.queue_graphics_index: %u", out.queue_graphics_index);
	infol(vulkan_device, "out.queue_transfer_index: %u", out.queue_transfer_index);
	infol(vulkan_device, "out.queue_compute_index: %u", out.queue_compute_index);
	infol(vulkan_device, "out.queue_present_index: %u", out.queue_present_index);

	f32 queue_priority = 1.0f;
	vk_device_queue_ci_da queue_ci_da = {0};
	da_declare(u32, unique_queue_da); 
	unique_queue_da unique_queues = {0};

	{
		bool8 graphics_index_present = false;
		for da_each(&unique_queues, it)
		{
			if(*it == out.queue_graphics_index)
			{
				graphics_index_present = true;
				break;
			}
		}
		if(!graphics_index_present)
		{
			da_append(temp_arena, &unique_queues, out.queue_graphics_index);
		}
	}

	{
		bool8 transfer_index_present = false;
		for da_each(&unique_queues, it)
		{
			if(*it == out.queue_transfer_index)
			{
				transfer_index_present = true;
				break;
			}
		}
		if(!transfer_index_present)
		{
			da_append(temp_arena, &unique_queues, out.queue_transfer_index);
		}
	}

	{
		bool8 compute_index_present = false;
		for da_each(&unique_queues, it)
		{
			if(*it == out.queue_compute_index)
			{
				compute_index_present = true;
				break;
			}
		}
		if(!compute_index_present)
		{
			da_append(temp_arena, &unique_queues, out.queue_compute_index);
		}
	}

	{
		bool8 present_index_present = false;
		for da_each(&unique_queues, it)
		{
			if(*it == out.queue_present_index)
			{
				present_index_present = true;
				break;
			}
		}
		if(!present_index_present)
		{
			da_append(temp_arena, &unique_queues, out.queue_present_index);
		}
	}
	
	for da_each(&unique_queues, it)
	{
		VkDeviceQueueCreateInfo ci = 
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = *it,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority
			};

		da_append(temp_arena, &queue_ci_da, ci);
	}


	cstr_da device_extensions = {0}; 

	da_append(temp_arena, &device_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	da_append(temp_arena, &device_extensions, VK_KHR_MAINTENANCE3_EXTENSION_NAME);
	da_append(temp_arena, &device_extensions, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);			
	
	#ifdef __APPLE__
		da_append(temp_arena, &device_extensions, "VK_KHR_portability_subset");			
	#endif	

	validate_device_extensions(temp_arena, pdevice, &device_extensions);

	VkPhysicalDeviceFeatures device_features = {0};

	device_features.samplerAnisotropy = VK_TRUE;	

	VkPhysicalDeviceDescriptorIndexingFeatures indexing_feature = {0};
	indexing_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexing_feature.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	indexing_feature.descriptorBindingPartiallyBound = VK_TRUE;
	indexing_feature.runtimeDescriptorArray = VK_TRUE;
	indexing_feature.descriptorBindingVariableDescriptorCount = VK_TRUE;
	indexing_feature.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	indexing_feature.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

	VkPhysicalDeviceFeatures2 features2 = {0};

	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &indexing_feature;

	vkGetPhysicalDeviceFeatures2(pdevice->handle, &features2);

	VkDeviceCreateInfo device_ci = {0};
	device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
	device_ci.pNext = &features2;
	device_ci.pQueueCreateInfos = queue_ci_da.data;
	device_ci.queueCreateInfoCount = queue_ci_da.occupied;

	device_ci.ppEnabledExtensionNames = device_extensions.data;
	device_ci.enabledExtensionCount = device_extensions.occupied;

	device_ci.pEnabledFeatures = NULL;

	vk_check(vkCreateDevice(pdevice->handle, &device_ci, NULL, &out.handle));	

	return out;
}

void vk_device_release(vk_device *device)
{
	vkDestroyDevice(device->handle, NULL);
}