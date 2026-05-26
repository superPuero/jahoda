#include "render_target_manager.h"

render_target_manager render_target_manager_make(vk_device* device, vk_physical_device* pdevice)
{
	// dummy image to get memory requrements
	VkImageCreateInfo dummy_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM, 
		.extent = {1, 1, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage dummy_image;
	vk_check(vkCreateImage(device->handle, &dummy_ci, NULL, &dummy_image));

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(device->handle, dummy_image, &mem_reqs);

	u32 render_target_memory_index = vk_physical_device_pick_memory_type(
		pdevice, 
		mem_reqs.memoryTypeBits,             
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  
	);

	vkDestroyImage(device->handle, dummy_image, NULL);
	// --------------------------------------

	// ------------ vulkan 1.1 way --------------- 	
	// VkDeviceImageMemoryRequirements image_reqs_info = {
   	// 	.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
	// 	.pCreateInfo = &dummy_ci
	// };

	// VkMemoryRequirements2 mem_reqs2 = {
	// 	.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
	// };

	// vkGetDeviceImageMemoryRequirements(device->handle, &image_reqs_info, &mem_reqs2);

	// u32 render_target_memory_index = vk_physical_device_pick_memory_type(
	// 	pdevice, 
	// 	mem_reqs2.memoryRequirements.memoryTypeBits, 
	// 	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	// );
	// ------------------------------------------

	return (render_target_manager){
		.win_sized_render_targets_arena = gpu_arena_make(
			device, 
			jahoda_win_sized_render_target_arena_mem, 
			render_target_memory_index, 
			strv_from_cstr("win_sized_render_target_arena"),
			false
		)			
	};
}

void render_target_manager_release(vk_device* device, render_target_manager* manager)
{
	gpu_arena_release(device, &manager->win_sized_render_targets_arena);
}