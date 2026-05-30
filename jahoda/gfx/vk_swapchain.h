#ifndef jahoda_vk_swapchain
#define jahoda_vk_swapchain

#include "vk_instance.h"
#include "vk_device.h"
#include "vk_device.h"

typedef struct
{
	VkSwapchainKHR handle;	
	VkFormat format;
	VkExtent2D extent;
	u32 image_count; 
	VkImage images[jahoda_vk_max_swapchain_image_count];
	VkImageView image_views[jahoda_vk_max_swapchain_image_count];
} vk_swapchain;

vk_swapchain vk_swapchain_make(vk_device *device, vk_surface *surface);
void vk_swapchain_release(vk_device *device, vk_swapchain *swapchain);

#endif