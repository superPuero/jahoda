#include "gpu_swapchain.h"

gpu_swapchain gpu_swapchain_make(gpu_device *device, gpu_surface *surface, arena *temp_mem, VkPresentModeKHR present_mode, gpu_swapchain *old_swapchain)
{
	gpu_swapchain out = { .present_mode = present_mode };	

	b8 supports = gpu_surface_supports_present_format(surface, device->pdevice, temp_mem, present_mode);

	dbg_verify(supports, "requested present mode %u is not supported", out.present_mode);

	out.format = surface->surface_format.format;

	// can do that only because surface refreshed its cabailities
	out.extent = surface->capabilities.currentExtent;

	VkSwapchainCreateInfoKHR swapchain_ci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchain_ci.surface = surface->handle;
	swapchain_ci.minImageCount = surface->capabilities.minImageCount + 1;
	swapchain_ci.imageFormat = surface->surface_format.format;
	swapchain_ci.imageColorSpace = surface->surface_format.colorSpace;
	swapchain_ci.imageExtent = out.extent;
	swapchain_ci.imageArrayLayers = 1;	
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	u32 queue_family_indices[] = { device->queue_graphics_index, device->queue_present_index };
	if (device->queue_graphics_index != device->queue_present_index)
	{
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.queueFamilyIndexCount = 2;
		swapchain_ci.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_ci.queueFamilyIndexCount = 0;
		swapchain_ci.pQueueFamilyIndices = NULL;
	}

	swapchain_ci.preTransform = surface->capabilities.currentTransform;
	swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_ci.presentMode = present_mode;
	swapchain_ci.clipped = VK_TRUE;
	swapchain_ci.oldSwapchain = old_swapchain ? old_swapchain->handle : NULL;

	gpu_check(vkCreateSwapchainKHR(device->handle, &swapchain_ci, NULL, &out.handle));

	gpu_check(vkGetSwapchainImagesKHR(device->handle, out.handle, &out.image_count, NULL));

	verifyl(
		gpu, 
		out.image_count >= frames_in_flight, 
		"expected atleast %u images, got %u", frames_in_flight, out.image_count
	);

	infol(gpu, "fetched %u images from swapchain", out.image_count);

	VkImage images[gpu_max_swapchain_image_count];
	VkImageView image_views[gpu_max_swapchain_image_count];

	gpu_check(vkGetSwapchainImagesKHR(device->handle, out.handle, &out.image_count, images));

	VkImageSubresourceRange range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
		.baseMipLevel = 0,                       
		.levelCount = 1,                         
		.baseArrayLayer = 0,                     
		.layerCount = 1                          
	};

	for(uz i = 0; i < out.image_count; i++)
	{
		VkImageViewCreateInfo image_view_ci = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = images[i], 
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.subresourceRange = range,
			.format = out.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			}
		};
		
		gpu_check(vkCreateImageView(device->handle, &image_view_ci, NULL, image_views + i));
	}

	for each_index(i, out.image_count)
	{
		out.textures[i].tag = text128_make("swaphcain.textures[%u]", i);
		out.textures[i].handle = images[i];
		out.textures[i].view_handle = image_views[i];
		out.textures[i].state = gpu_texture_state_undefined;
		out.textures[i].format = out.format;
	}

	return out;
}

void gpu_swapchain_release(gpu_device *device, gpu_swapchain *swapchain)
{
	for each_index(i, swapchain->image_count)
	{
		vkDestroyImageView(device->handle, swapchain->textures[i].view_handle, NULL);		
	}

	vkDestroySwapchainKHR(device->handle, swapchain->handle, NULL);
}