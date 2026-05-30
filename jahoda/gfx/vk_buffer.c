// #include "vk_buffer.h"

// VkBuffer vk_buffer_imm_create_and_upload(vk_device *device, )
// {
// 	vk_check(vkCreateBuffer(gpu->device.handle, &buffer_ci, NULL, &staging_buffer));
// 	VkMemoryRequirements buf_mem_reqs;
// 	vkGetBufferMemoryRequirements(gpu->device.handle, staging_buffer, &buf_mem_reqs);
	
// 	u32 memory_type_index = vk_physical_device_pick_memory_type(
// 		&gpu->pdevice, 
// 		buf_mem_reqs.memoryTypeBits, 
// 		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
// 	);
	
// 	VkMemoryAllocateInfo alloc_info = {
// 		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
// 		.allocationSize = buf_mem_reqs.size,
// 		.memoryTypeIndex = memory_type_index
// 	};
	
// 	char *data;
// 	VkDeviceMemory staging_memory;
// 	vk_check(vkAllocateMemory(gpu->device.handle, &alloc_info, NULL, &staging_memory));
// 	vk_check(vkBindBufferMemory(gpu->device.handle, staging_buffer, staging_memory, 0));
	
// 	vk_check(vkMapMemory(gpu->device.handle, staging_memory, 0, image_size, 0, &data));
// 	memcpy(data, font->atlas_pixels, image_size);
// 	vkUnmapMemory(gpu->device.handle, staging_memory);
// }

