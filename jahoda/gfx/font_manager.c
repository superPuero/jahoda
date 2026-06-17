#include "font_manager.h"

font_id font_manager_load(font_manager *manager, gpu_context *gpu, arena *static_arena, arena *scratch_arena, strv font_path)
{
	verify(manager->next_font_id < jahoda_max_font_count);

	font_id id = manager->next_font_id++;
	font_manager_entry *entry = &manager->entries[id];

	entry->altas = font_atlas_make(static_arena, scratch_arena, font_path);

	VkDeviceSize image_size = entry->altas.atlas_width * entry->altas.atlas_height;

	VkImageCreateInfo image_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8_UNORM, 
        .extent = { entry->altas.atlas_width, entry->altas.atlas_height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    vk_check(vkCreateImage(gpu->device.handle, &image_ci, NULL, &entry->tex.image));

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(gpu->device.handle, entry->tex.image, &mem_reqs);

	VkDeviceSize image_alloc = gpu_arena_push(
		&gpu->static_dl_img_arenas[gpu->frame_index], 
		image_size, 
		mem_reqs.alignment
	);

    vk_check(vkBindImageMemory(gpu->device.handle, entry->tex.image, gpu->static_dl_img_arenas[gpu->frame_index].mem, image_alloc));

    VkImageViewCreateInfo view_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = entry->tex.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk_check(vkCreateImageView(gpu->device.handle, &view_ci, NULL, &entry->tex.view));

	VkBuffer staging_buffer;

	VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = image_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };	 

	vk_check(vkCreateBuffer(gpu->device.handle, &buffer_ci, NULL, &staging_buffer));
	VkMemoryRequirements buf_mem_reqs;
    vkGetBufferMemoryRequirements(gpu->device.handle, staging_buffer, &buf_mem_reqs);
		
	VkDeviceSize stagin_buf_alloc_offset = gpu_arena_push(
		&gpu->temp_hc_buf_arenas[gpu->frame_index],  
		buf_mem_reqs.size, 
		buf_mem_reqs.alignment
	);

	memcpy(
		gpu->temp_hc_buf_arenas[gpu->frame_index].mapped_ptr + stagin_buf_alloc_offset, 
		entry->altas.atlas_pixels, 
		image_size
	);

	vk_check(vkBindBufferMemory(
		gpu->device.handle, 
		staging_buffer, 
		gpu->temp_hc_buf_arenas[gpu->frame_index].mem, 
		stagin_buf_alloc_offset
	));

	VkCommandBuffer cmd = gpu_context_grab_graphics(gpu); // @explain graphics buffers are also transfer capable

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vk_check(vkBeginCommandBuffer(cmd, &begin_info));

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = entry->tex.image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
    };

	// VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { entry->altas.atlas_width, entry->altas.atlas_height, 1 }
    };

    vkCmdCopyBufferToImage(cmd, staging_buffer, entry->tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
	// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	vk_check(vkEndCommandBuffer(cmd));

	gpu_context_immidiate_graphics(gpu, cmd);
	
	vkDestroyBuffer(gpu->device.handle, staging_buffer, NULL);

	return id;
}

void font_manager_release(font_manager *manager, gpu_context *gpu)
{
	for(uz i = 0; i < manager->next_font_id; i++)
	{		
		texture_release(&gpu->device, &manager->entries[i].tex);
	}
}