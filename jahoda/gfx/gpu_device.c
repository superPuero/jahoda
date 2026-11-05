#include "gpu_device.h"
#include <jahoda/core/da.h>

#define gpu_load_pfn(dev, fn) (PFN_##fn)vkGetDeviceProcAddr((dev), #fn)

internal 
void validate_device_extensions(arena *temp_arena, gpu_physical_device *pdevice, cstr_da *device_extensions)
{
	u32 device_extension_count;
	vkEnumerateDeviceExtensionProperties(pdevice->handle, NULL, &device_extension_count, NULL);

	gpu_exntension_propertioes_da avalible_device_extensions = {0};
	da_resize(&avalible_device_extensions, temp_arena, device_extension_count);
	vkEnumerateDeviceExtensionProperties(pdevice->handle, NULL, &device_extension_count, avalible_device_extensions.data);

	for da_each(device_extensions_it, device_extensions)
	{		
		b8 found = false;
		for da_each(avalible_device_extensions_id, &avalible_device_extensions)
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

gpu_device gpu_device_make(arena *temp_arena, gpu_physical_device *pdevice, gpu_surface *surface)
{
	gpu_device out = {0};
	out.queue_graphics_index = jahoda_gpu_invalid_queue_family;
	out.queue_transfer_index = jahoda_gpu_invalid_queue_family;
	out.queue_compute_index = jahoda_gpu_invalid_queue_family;
	out.queue_present_index = jahoda_gpu_invalid_queue_family;

	u32 queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pdevice->handle, &queue_families_count, NULL);

	gpu_queue_family_properties_da queue_families = {0};
	da_resize(&queue_families, temp_arena, queue_families_count);

	vkGetPhysicalDeviceQueueFamilyProperties(pdevice->handle, &queue_families_count, queue_families.data);
		
	for (u32 i = 0; i < queue_families.occupied; i++)
	{
		VkQueueFamilyProperties family = queue_families.data[i];

		if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT
			&& out.queue_graphics_index == jahoda_gpu_invalid_queue_family)
		{
			out.queue_graphics_index = i;
		}

		if (family.queueFlags & VK_QUEUE_TRANSFER_BIT
			&& out.queue_graphics_index != i
			&& out.queue_transfer_index == jahoda_gpu_invalid_queue_family)
		{
			out.queue_transfer_index = i;
		}

		if (family.queueFlags & VK_QUEUE_COMPUTE_BIT
			&& out.queue_graphics_index != i
			&& out.queue_transfer_index != i
			&& out.queue_compute_index == jahoda_gpu_invalid_queue_family)
		{
			out.queue_compute_index = i;
		}

		VkBool32 present_supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(pdevice->handle, i, surface->handle, &present_supported);

		if (present_supported
			&& out.queue_compute_index != i
			&& out.queue_graphics_index != i
			&& out.queue_transfer_index != i
			&& out.queue_present_index == jahoda_gpu_invalid_queue_family
			)
		{
			out.queue_present_index = i;
		}

		if (out.queue_graphics_index != jahoda_gpu_invalid_queue_family
			&& out.queue_transfer_index != jahoda_gpu_invalid_queue_family
			&& out.queue_compute_index != jahoda_gpu_invalid_queue_family
			&& out.queue_present_index != jahoda_gpu_invalid_queue_family)
		{
			break;
		}
	}

	if (out.queue_compute_index == jahoda_gpu_invalid_queue_family)
	{
		infol(gpu, "wasnt able to find dedicated compute queue");

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

	if (out.queue_present_index == jahoda_gpu_invalid_queue_family)
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

	dbg_verify(out.queue_graphics_index != jahoda_gpu_invalid_queue_family, "");
	dbg_verify(out.queue_transfer_index != jahoda_gpu_invalid_queue_family, "");
	dbg_verify(out.queue_compute_index != jahoda_gpu_invalid_queue_family, "");
	dbg_verify(out.queue_present_index != jahoda_gpu_invalid_queue_family, "");

	infol(gpu, "out.queue_graphics_index: %u", out.queue_graphics_index);
	infol(gpu, "out.queue_transfer_index: %u", out.queue_transfer_index);
	infol(gpu, "out.queue_compute_index: %u", out.queue_compute_index);
	infol(gpu, "out.queue_present_index: %u", out.queue_present_index);

	out.queue_graphics_timestamp_valid_bits = queue_families.data[out.queue_graphics_index].timestampValidBits;

	f32 queue_priority = 1.0f;
	gpu_device_queue_ci_da queue_ci_da = {0};
	da_declare(u32, unique_queue_da); 
	unique_queue_da unique_queues = {0};
	
	if(!da_has(&unique_queues, out.queue_graphics_index)) 
	{ 
		da_append(&unique_queues, temp_arena, out.queue_graphics_index);
	}

	if(!da_has(&unique_queues, out.queue_transfer_index)) 
	{ 
		da_append(&unique_queues, temp_arena, out.queue_transfer_index);
	}	

	if(!da_has(&unique_queues, out.queue_compute_index)) 
	{ 
		da_append(&unique_queues, temp_arena, out.queue_compute_index);
	}

	if(!da_has(&unique_queues, out.queue_present_index)) 
	{ 
		da_append(&unique_queues, temp_arena, out.queue_present_index);
	}	
	
	for da_each(it, &unique_queues)
	{
		VkDeviceQueueCreateInfo ci = 
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = *it,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority
			};

		da_append(&queue_ci_da, temp_arena, ci);
	}

	cstr_da device_extensions = {0}; 

	da_append(&device_extensions, temp_arena, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	da_append(&device_extensions, temp_arena, VK_KHR_MAINTENANCE3_EXTENSION_NAME);
	da_append(&device_extensions, temp_arena, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	da_append(&device_extensions, temp_arena, VK_EXT_DEVICE_FAULT_EXTENSION_NAME);	
	da_append(&device_extensions, temp_arena, VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);	

	#ifdef __APPLE__
		da_append(temp_arena, &device_extensions, "VK_KHR_portability_subset");			
	#endif	

	validate_device_extensions(temp_arena, pdevice, &device_extensions);

	
	// VkPhysicalDeviceRobustness2FeaturesKHR robustness2 ={
	// 	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR,
	// 	.nullDescriptor = VK_TRUE
	// };

	VkPhysicalDeviceVulkan13Features vk13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.dynamicRendering = VK_TRUE,
		.synchronization2 = VK_TRUE,
		// .pNext = &robustness2			
	};

	VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .descriptorBuffer = VK_TRUE, 
		.descriptorBufferCaptureReplay = VK_TRUE,
        .pNext = &vk13_features
    };

	VkPhysicalDeviceVulkan12Features vk12_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
    	.bufferDeviceAddressCaptureReplay = VK_TRUE,
        .scalarBlockLayout = VK_TRUE,
        
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        
        // Link the descriptor buffer features at the end of the chain
        .pNext = &descriptor_buffer_features 
    };

	VkPhysicalDeviceFeatures2 features2 = {0};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &vk12_features;	
	features2.features.samplerAnisotropy = VK_TRUE;
	features2.features.multiDrawIndirect = VK_TRUE;
	features2.features.drawIndirectFirstInstance = VK_TRUE;
	
	VkDeviceCreateInfo device_ci = {0};
	device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
	device_ci.pNext = &features2;
	device_ci.pQueueCreateInfos = queue_ci_da.data;
	device_ci.queueCreateInfoCount = queue_ci_da.occupied;

	device_ci.ppEnabledExtensionNames = device_extensions.data;
	device_ci.enabledExtensionCount = device_extensions.occupied;

	device_ci.pEnabledFeatures = NULL;

	gpu_check(vkCreateDevice(pdevice->handle, &device_ci, NULL, &out.handle));	

	out.pdevice = pdevice;

	out.get_descriptor_set_layout_size = gpu_load_pfn(out.handle, vkGetDescriptorSetLayoutSizeEXT);
	out.get_descriptor_set_layout_binding_offset = gpu_load_pfn(out.handle, vkGetDescriptorSetLayoutBindingOffsetEXT);
	out.get_descriptor = gpu_load_pfn(out.handle, vkGetDescriptorEXT);
	out.cmd_bind_descriptor_buffer = gpu_load_pfn(out.handle, vkCmdBindDescriptorBuffersEXT);
	out.cmd_set_descriptor_buffer_offset = gpu_load_pfn(out.handle, vkCmdSetDescriptorBufferOffsetsEXT);

	out.get_device_fault_info = gpu_load_pfn(out.handle, vkGetDeviceFaultInfoEXT);
	
	return out;
}

void gpu_device_release(gpu_device *device)
{
	vkDestroyDevice(device->handle, NULL);
}

void gpu_device_wait_idle(gpu_device *device)
{
	gpu_check(vkDeviceWaitIdle(device->handle));
}

const char *gpu_device_fault_address_type_to_cstr(VkDeviceFaultAddressTypeEXT type)
{
	switch (type) {
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_NONE_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT);
		stringify_case(VK_DEVICE_FAULT_ADDRESS_TYPE_MAX_ENUM_EXT);
	}
}
