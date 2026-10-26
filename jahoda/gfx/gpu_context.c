#include "gpu_context.h"

gpu_context gpu_context_make_(gpu_context_params params)
{
	gpu_context out = {  .present_mode = params.present_mode};

	out.instance = gpu_instance_make(params.temp_arena);
	
	out.pdevice = arena_ppush(params.static_arena, gpu_physical_device);
	*out.pdevice = gpu_physical_device_make(params.temp_arena, params.static_arena, &out.instance);

	out.surface = gpu_surface_make(params.temp_arena, params.win, &out.instance, out.pdevice);
	out.device = gpu_device_make(params.temp_arena, out.pdevice, &out.surface);
	out.swapchain_needs_refresh = false;
	out.swapchain_was_refreshed = true;

	out.swapchain = gpu_swapchain_make(&out.device, &out.surface, params.temp_arena, out.present_mode, NULL);
	verifyl(gpu_context_verify_fail, out.swapchain.image_count >= frames_in_flight, "");

	vkGetDeviceQueue(out.device.handle, out.device.queue_graphics_index, 0, &out.graphics_queue);
	vkGetDeviceQueue(out.device.handle, out.device.queue_transfer_index, 0, &out.transfer_queue);
	vkGetDeviceQueue(out.device.handle, out.device.queue_present_index, 0, &out.present_queue);

	VkCommandPoolCreateInfo graphics_pool_ci = { 
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = out.device.queue_graphics_index,
	};

	gpu_check(vkCreateCommandPool(out.device.handle, &graphics_pool_ci, NULL, &out.graphics_pool));

	VkCommandPoolCreateInfo transfer_pool_ci = { 
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = out.device.queue_transfer_index,
	};

	gpu_check(vkCreateCommandPool(out.device.handle, &transfer_pool_ci, NULL, &out.transfer_pool));
	
	
	for(uz i = 0; i < gpu_max_swapchain_image_count; ++i)
	{
		VkSemaphoreCreateInfo semaphore_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		gpu_check(vkCreateSemaphore(out.device.handle, &semaphore_ci, NULL, out.graphics_semaphores + i));
	}
	
	
	for(uz i = 0; i < frames_in_flight; i++)
	{								
		VkSemaphoreCreateInfo semaphore_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		
		gpu_check(vkCreateSemaphore(out.device.handle, &semaphore_ci, NULL, out.image_avalible_semaphores + i));

		VkFenceCreateInfo fence_ci = { 			
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		gpu_check(vkCreateFence(out.device.handle, &fence_ci, NULL, out.render_fences + i));

		VkCommandBufferAllocateInfo gfx_cmd_buf_alloc = { 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = out.graphics_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = jahoda_gpu_context_max_graphics_cmd_count
		};
	
		gpu_check(
			vkAllocateCommandBuffers(
				out.device.handle, 
				&gfx_cmd_buf_alloc, 
				out.graphics_buffers[i]
			)
		);		
	}

	return out;
}

void gpu_context_release(gpu_context *gpu)
{
	vkDestroyCommandPool(gpu->device.handle, gpu->graphics_pool, NULL);
	vkDestroyCommandPool(gpu->device.handle, gpu->transfer_pool, NULL);

	for(uz i = 0; i < gpu_max_swapchain_image_count; ++i)
	{
		vkDestroySemaphore(gpu->device.handle, gpu->graphics_semaphores[i], NULL);
	}
	
	for(uz i = 0; i < frames_in_flight; ++i)
	{
		vkDestroySemaphore(gpu->device.handle, gpu->image_avalible_semaphores[i], NULL);
		vkDestroyFence(gpu->device.handle, gpu->render_fences[i], NULL);
	}

	gpu_swapchain_release(&gpu->device, &gpu->swapchain);
	gpu_device_release(&gpu->device);
	gpu_surface_release(&gpu->instance, &gpu->surface);
	gpu_instance_release(&gpu->instance);
}

void gpu_context_set_present_mode(gpu_context *gpu, VkPresentModeKHR present_mode)
{
	if(gpu->present_mode != present_mode)
	{
		gpu->present_mode = present_mode;
		gpu->swapchain_needs_refresh = true;
	}	
}

b8 gpu_context_try_begin_frame(gpu_context *gpu, window *win, arena *temp_mem)
{
	vkWaitForFences(
		gpu->device.handle, 
		1, 
		gpu->render_fences + gpu->frame_index, 
		true,
		UINT64_MAX
	); 

	gpu->commited_graphics_count[gpu->frame_index] = 0;
	gpu->next_graphics_buffer[gpu->frame_index] = 0;
	
	if(gpu->swapchain_was_refreshed)
	{
		gpu->swapchain_was_refreshed = false;
	}

	if (gpu->swapchain_needs_refresh)
	{
		vkDeviceWaitIdle(gpu->device.handle);
		gpu_surface_refresh_capabilities(&gpu->surface, gpu->pdevice);		
		gpu_swapchain new_swapchain = gpu_swapchain_make(&gpu->device, &gpu->surface, temp_mem, gpu->present_mode, &gpu->swapchain);		
		gpu_swapchain_release(&gpu->device, &gpu->swapchain);
		gpu->swapchain = new_swapchain;
		gpu->swapchain_needs_refresh = false;
		gpu->swapchain_was_refreshed = true;
		gpu->swapchain_image_index = 0;
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

	switch(res)
	{
		case VK_SUCCESS:
			return true;
			
		case VK_SUBOPTIMAL_KHR:
			gpu->swapchain_needs_refresh = true;
			return true;
			
		case VK_ERROR_OUT_OF_DATE_KHR:
			gpu->swapchain_needs_refresh = true;
			return false;			

		default:
			gpu_check(res);
			return false; 
	} 
}

void gpu_context_end_frame(gpu_context *gpu, arena *temp_mem)
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
	gpu_check_2(vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, gpu->render_fences[gpu->frame_index]), &gpu->device, temp_mem);

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
		gpu_check(present_result);
	}

	gpu->frame_index = (gpu->frame_index + 1) % frames_in_flight;
}

VkCommandBuffer gpu_context_grab_graphics(gpu_context *gpu)
{
	dbg_verify(gpu->next_graphics_buffer[gpu->frame_index] <= jahoda_gpu_context_max_graphics_cmd_count, "graphics command buffer limit reached");

	VkCommandBuffer cmd = gpu->graphics_buffers[gpu->frame_index][gpu->next_graphics_buffer[gpu->frame_index]++];

	gpu_check(
		vkBeginCommandBuffer(
			cmd, 
			&(VkCommandBufferBeginInfo){
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			}
		)
	);

	return cmd;
}

void gpu_context_commit_graphics(gpu_context *gpu, VkCommandBuffer cmd)
{
	gpu_check(vkEndCommandBuffer(cmd));
	gpu->commited_graphics_buffers[gpu->frame_index][gpu->commited_graphics_count[gpu->frame_index]++] = cmd;
}

void gpu_context_immidiate_graphics(gpu_context *gpu)
{
	VkSubmitInfo submit_info = { 0 };
	
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount		= 0;
	submit_info.commandBufferCount		= gpu->commited_graphics_count[gpu->frame_index];
	submit_info.pCommandBuffers			= gpu->commited_graphics_buffers[gpu->frame_index];
	
	gpu_check(vkQueueSubmit(gpu->graphics_queue, 1, &submit_info, 0));
    gpu_check(vkQueueWaitIdle(gpu->graphics_queue));
	gpu->commited_graphics_count[gpu->frame_index] = 0;
	gpu->next_graphics_buffer[gpu->frame_index] = 0;
}
