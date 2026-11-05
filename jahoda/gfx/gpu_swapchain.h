#ifndef jahoda_gpu_swapchain
#define jahoda_gpu_swapchain

#include "gpu_device.h"
#include "gpu_device.h"
#include "gpu_texture.h"

#define gpu_max_swapchain_image_count 16

typedef struct
{
	VkSwapchainKHR handle;	
	VkFormat format;
	VkExtent2D extent;
	u32 image_count; 
	gpu_texture textures[gpu_max_swapchain_image_count];
	VkPresentModeKHR present_mode;
} gpu_swapchain;
gpu_swapchain gpu_swapchain_make(gpu_device *device, gpu_surface *surface, arena *temp_mem, VkPresentModeKHR present_mode, gpu_swapchain *old_swapchain);
void gpu_swapchain_release(gpu_device *device, gpu_swapchain *swapchain);

#endif