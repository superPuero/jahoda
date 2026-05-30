#include "vk_swapchain.h"

vk_swapchain vk_swapchain_make(vk_device *device, vk_surface *surface)
{
	vk_swapchain out = {0};
	
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
	swapchain_ci.presentMode = surface->present_mode;
	swapchain_ci.clipped = VK_TRUE;
	swapchain_ci.oldSwapchain = VK_NULL_HANDLE;

	vk_check(vkCreateSwapchainKHR(device->handle, &swapchain_ci, NULL, &out.handle));

	vk_check(vkGetSwapchainImagesKHR(device->handle, out.handle, &out.image_count, NULL));

	verifyl(
		swapchain_verify_fail, 
		out.image_count >= jahoda_vk_frames_in_flight, 
		"expected atleast %u images, got %u", jahoda_vk_frames_in_flight, out.image_count
	);

	vk_check(vkGetSwapchainImagesKHR(device->handle, out.handle, &out.image_count, out.images));

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
			.image = out.images[i], 
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
		
		vk_check(vkCreateImageView(device->handle, &image_view_ci, NULL, out.image_views + i));
	}


	return out;
}

void vk_swapchain_release(vk_device *device, vk_swapchain *swapchain)
{
	for (uz i = 0; i < swapchain->image_count; i++)
    {
		vkDestroyImageView(device->handle, swapchain->image_views[i], NULL);
		swapchain->image_views[i] = VK_NULL_HANDLE; 
    }

	vkDestroySwapchainKHR(device->handle, swapchain->handle, NULL);
}