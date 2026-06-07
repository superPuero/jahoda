#include "gpu_context.h"

gpu_context gpu_context_make(arena temp_arena, arena static_arena, window *win)
{
	gpu_context out = {0};

	out.instance = vk_instance_make(temp_arena);
	out.pdevice = vk_physical_device_make(temp_arena, static_arena, &out.instance);
	out.surface = vk_surface_make(temp_arena, win, &out.instance, &out.pdevice);

	out.device = vk_device_make(temp_arena, &out.pdevice, &out.surface);

	out.swapchain = vk_swapchain_make(&out.device, &out.surface);
	verifyl(gpu_context_verify_fail, out.swapchain.image_count >= jahoda_vk_frames_in_flight, "");

	vkGetDeviceQueue(out.device.handle, out.device.queue_graphics_index, 0, &out.graphics_queue);
	vkGetDeviceQueue(out.device.handle, out.device.queue_transfer_index, 0, &out.transfer_queue);
	vkGetDeviceQueue(out.device.handle, out.device.queue_present_index, 0, &out.present_queue);

	VkCommandPoolCreateInfo graphics_pool_ci = { 
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = out.device.queue_graphics_index,
	};

	vk_check(vkCreateCommandPool(out.device.handle, &graphics_pool_ci, NULL, &out.graphics_pool));

	VkCommandPoolCreateInfo transfer_pool_ci = { 
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = out.device.queue_transfer_index,
	};

	vk_check(vkCreateCommandPool(out.device.handle, &transfer_pool_ci, NULL, &out.transfer_pool));
	
	
	for(uz i = 0; i < jahoda_vk_max_swapchain_image_count; ++i)
	{
		VkSemaphoreCreateInfo semaphore_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		vk_check(vkCreateSemaphore(out.device.handle, &semaphore_ci, NULL, out.graphics_semaphores + i));
	}

	u32 host_visible_buffer_memory_type_index;
	// getting meory type index of host visible | coherent buffer memory via dummy bufferpfif_arenas
	{
		VkBuffer dummy_buffer;
		VkBufferCreateInfo buffer_ci = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = 1,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		vk_check(vkCreateBuffer(out.device.handle, &buffer_ci, NULL, &dummy_buffer));
		VkMemoryRequirements buf_mem_reqs;
		vkGetBufferMemoryRequirements(out.device.handle, dummy_buffer, &buf_mem_reqs);
		vkDestroyBuffer(out.device.handle, dummy_buffer, NULL);
		
		host_visible_buffer_memory_type_index = vk_physical_device_pick_memory_type(
			&out.pdevice, 
			buf_mem_reqs.memoryTypeBits, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
	}

	u32 render_target_memory_index;
	// getting device only image memory type index via dummy image 		
	{		
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
		vk_check(vkCreateImage(out.device.handle, &dummy_ci, NULL, &dummy_image));

		VkMemoryRequirements mem_reqs;
		vkGetImageMemoryRequirements(out.device.handle, dummy_image, &mem_reqs);

		render_target_memory_index = vk_physical_device_pick_memory_type(
			&out.pdevice, 
			mem_reqs.memoryTypeBits,             
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  
		);

		vkDestroyImage(out.device.handle, dummy_image, NULL);
	}

	
	for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
	{		
		
		str temp_hc_buf_arena_name = str_from_fmt(temp_arena, "temp_hc_buf_arenas[%zu]", i);		
				
		out.temp_hc_buf_arenas[i] = gpu_arena_make(
			&out.device, 
			temp_host_coherent_buffer_arena_size, 
			host_visible_buffer_memory_type_index, 
			strv_from_str(&temp_hc_buf_arena_name), 
			true 
		); 		

		str static_dl_sw_img_arena_name = str_from_fmt(temp_arena, "static_dl_sw_img_arenas[%zu]", i);
				
		out.static_dl_sw_img_arenas[i] = gpu_arena_make(
			&out.device, 
			static_device_local_swapchain_image_arena_size, // @todo: tweak around 
			render_target_memory_index, 
			strv_from_str(&static_dl_sw_img_arena_name),
			false
		);

		str static_dl_img_arena_name = str_from_fmt(temp_arena, "static_dl_img_arenas[%zu]", i);

		out.static_dl_img_arenas[i] = gpu_arena_make(
			&out.device, 
			Mb(32), // @todo: tweak around 
			render_target_memory_index, 
			strv_from_str(&static_dl_img_arena_name),
			false
		);
		
		VkSemaphoreCreateInfo semaphore_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		
		vk_check(vkCreateSemaphore(out.device.handle, &semaphore_ci, NULL, out.image_avalible_semaphores + i));

		VkFenceCreateInfo fence_ci = { 			
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		vk_check(vkCreateFence(out.device.handle, &fence_ci, NULL, out.render_fences + i));

		VkCommandBufferAllocateInfo gfx_cmd_buf_alloc = { 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = out.graphics_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = jahoda_gpu_context_max_graphics_cmd_count
		};
	
		vk_check(
			vkAllocateCommandBuffers(
				out.device.handle, 
				&gfx_cmd_buf_alloc, 
				out.graphics_buffers[i]
			)
		);
		
		VkCommandBufferAllocateInfo transfer_cmd_buf_alloc = { 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = out.transfer_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = jahoda_gpu_context_max_graphics_cmd_count
		};

		vk_check(
			vkAllocateCommandBuffers(
				out.device.handle, 
				&transfer_cmd_buf_alloc, 
				out.transfer_buffers[i]
			)
		);
	}

	return out;
}

void gpu_context_release(gpu_context *gpu)
{
	vkDestroyCommandPool(gpu->device.handle, gpu->graphics_pool, NULL);
	vkDestroyCommandPool(gpu->device.handle, gpu->transfer_pool, NULL);

	for(uz i = 0; i < jahoda_vk_max_swapchain_image_count; ++i)
	{
		vkDestroySemaphore(gpu->device.handle, gpu->graphics_semaphores[i], NULL);
	}

	for(uz i = 0; i < jahoda_vk_frames_in_flight; ++i)
	{
		gpu_arena_release(&gpu->device, gpu->temp_hc_buf_arenas + i);
		gpu_arena_release(&gpu->device, gpu->static_dl_sw_img_arenas + i); 
		gpu_arena_release(&gpu->device, gpu->static_dl_img_arenas + i); 

		vkDestroySemaphore(gpu->device.handle, gpu->image_avalible_semaphores[i], NULL);
		vkDestroyFence(gpu->device.handle, gpu->render_fences[i], NULL);
	}

	vk_swapchain_release(&gpu->device, &gpu->swapchain);
	vk_device_release(&gpu->device);
	vk_surface_release(&gpu->instance, &gpu->surface);
	vk_instance_release(&gpu->instance);
}

bool gpu_context_try_begin_frame(gpu_context *gpu, window *win)
{
	vkWaitForFences(
		gpu->device.handle, 
		1, 
		gpu->render_fences + gpu->frame_index, 
		true,
		UINT64_MAX
	); 

	gpu->temp_hc_buf_arenas[gpu->frame_index].current = 0;
	gpu->commited_graphics_count[gpu->frame_index] = 0;
	gpu->next_graphics_buffer[gpu->frame_index] = 0;
	gpu->next_transfer_buffer[gpu->frame_index] = 0;
	
	if(gpu->swapchain_was_refreshed)
	{
		gpu->swapchain_was_refreshed = false;
	}

	if (gpu->swapchain_needs_refresh)
	{
		vkDeviceWaitIdle(gpu->device.handle);
		vk_surface_refresh_capabilities(&gpu->surface, &gpu->pdevice);
		vk_swapchain_release(&gpu->device, &gpu->swapchain);
		gpu->swapchain = vk_swapchain_make(&gpu->device, &gpu->surface);
		gpu->swapchain_needs_refresh = false;
		gpu->swapchain_was_refreshed = true;
		gpu->swapchain_image_index = 0;

		for(uz i = 0; i < jahoda_vk_frames_in_flight; i++)
		{
			gpu->static_dl_sw_img_arenas[i].current = 0;
		}
	}

	VkResult res = vkAcquireNextImageKHR
	(
		gpu->device.handle,
		gpu->swapchain.handle,
		UINT64_MAX,
		gpu->image_avalible_semaphores[gpu->frame_index],
		VK_NULL_HANDLE,
		&gpu->swapchain_image_index
	);

	if (res == VK_SUBOPTIMAL_KHR)
	{
		gpu->swapchain_needs_refresh = true;
		return false;
	}
	else if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
		gpu->swapchain_needs_refresh = true;
		return false;
    }
	else if (res != VK_SUCCESS)
	{
		vk_check(res);
		return false;
	}

	return true;
}

void gpu_context_end_frame(gpu_context *gpu)
{
	VkSubmitInfo submit_info = { 0 };
	u32 mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount		= 1;
	submit_info.pWaitSemaphores			= gpu->image_avalible_semaphores + gpu->frame_index;
	submit_info.pWaitDstStageMask		= &mask;
	submit_info.commandBufferCount		= gpu->commited_graphics_count[gpu->frame_index];
	submit_info.pCommandBuffers			= gpu->commited_graphics_buffers[gpu->frame_index];
	submit_info.signalSemaphoreCount	= 1;
	submit_info.pSignalSemaphores		= gpu->graphics_semaphores + gpu->swapchain_image_index;

	vkResetFences(gpu->device.handle, 1, gpu->render_fences + gpu->frame_index);
	vk_check(vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, gpu->render_fences[gpu->frame_index]));

	VkPresentInfoKHR present_info = {0};	
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = gpu->graphics_semaphores + gpu->swapchain_image_index;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &gpu->swapchain.handle;
	present_info.pImageIndices = &gpu->swapchain_image_index;

	VkResult present_result = vkQueuePresentKHR(gpu->present_queue, &present_info);

	if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
	{
		gpu->swapchain_needs_refresh = true;
	}
	else if (present_result != VK_SUCCESS)
	{
		vk_check(present_result);
	}

	gpu->frame_index = (gpu->frame_index + 1) % jahoda_vk_frames_in_flight;
}

VkCommandBuffer gpu_context_grab_graphics(gpu_context *gpu)
{
	dbg_verify(gpu->next_graphics_buffer[gpu->frame_index] <= jahoda_gpu_context_max_graphics_cmd_count, "graphics command buffer limit reached");

	VkCommandBuffer cmd = gpu->graphics_buffers[gpu->frame_index][gpu->next_graphics_buffer[gpu->frame_index]++];

	vkResetCommandBuffer(cmd, 0);

	return cmd;
}

VkCommandBuffer gpu_context_grab_transfer(gpu_context *gpu)
{
	dbg_verify(gpu->next_transfer_buffer < jahoda_gpu_context_max_graphics_cmd_count, "transfer command buffer limit reached");

	VkCommandBuffer cmd = gpu->transfer_buffers[gpu->frame_index][gpu->next_transfer_buffer[gpu->frame_index]++];

	vkResetCommandBuffer(cmd, 0);

	return cmd;
}

void gpu_context_commit_graphics(gpu_context *gpu, VkCommandBuffer cmd)
{
	gpu->commited_graphics_buffers[gpu->frame_index][gpu->commited_graphics_count[gpu->frame_index]++] = cmd;
}

void gpu_context_immidiate_graphics(gpu_context *gpu, VkCommandBuffer cmd)
{
	VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };

    vk_check(vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    vk_check(vkQueueWaitIdle(gpu->graphics_queue));
}

void gpu_context_immidiate_transfer(gpu_context *gpu, VkCommandBuffer cmd)
{
	VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };

    vk_check(vkQueueSubmit(gpu->transfer_queue, 1, &submit_info, VK_NULL_HANDLE));

    vk_check(vkQueueWaitIdle(gpu->transfer_queue));
}