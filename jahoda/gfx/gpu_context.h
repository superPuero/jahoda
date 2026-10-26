#ifndef jahoda_gpu_context
#define jahoda_gpu_context

#include "gpu_instance.h"
#include "gpu_physical_device.h"
#include "gpu_surface.h"
#include "gpu_device.h"
#include "gpu_swapchain.h"
#include "gpu_arena.h"
#include "gpu_memory_pool.h"

#define jahoda_gpu_context_max_graphics_cmd_count 128

#define screen_buffer_mem_16_9(pixel_width, pixel_bit_size) (u32)(pixel_width * (pixel_width * 16/9) * (pixel_bit_size / 8))

typedef struct
{
	gpu_instance instance;
	gpu_physical_device *pdevice;
	gpu_surface surface;
	gpu_device device;	
	gpu_swapchain swapchain;
	
	VkQueue graphics_queue;
	VkQueue transfer_queue;
	VkQueue present_queue;
	
	u32 frame_index;
	u32 swapchain_image_index;	
	b8 swapchain_needs_refresh;
	b8 swapchain_was_refreshed;
	
	VkCommandPool graphics_pool;
	VkCommandPool transfer_pool;
		
	VkSemaphore graphics_semaphores[gpu_max_swapchain_image_count];
	
	VkFence render_fences[frames_in_flight];
	
	u32 next_graphics_buffer[frames_in_flight];
	VkCommandBuffer graphics_buffers[frames_in_flight][jahoda_gpu_context_max_graphics_cmd_count];
	
	u32 commited_graphics_count[frames_in_flight];
	VkCommandBuffer commited_graphics_buffers[frames_in_flight][jahoda_gpu_context_max_graphics_cmd_count];
	
	VkSemaphore image_avalible_semaphores[frames_in_flight];
	
	VkPresentModeKHR present_mode;
} gpu_context;	

typedef struct
{
	arena *temp_arena; 
	arena *static_arena;
	window *win;
	
	VkPresentModeKHR present_mode;
} gpu_context_params;

#define gpu_context_make(...) gpu_context_make_((gpu_context_params){__VA_ARGS__});
gpu_context gpu_context_make_(gpu_context_params params);

VkCommandBuffer gpu_context_grab_graphics(gpu_context *gpu);
void gpu_context_commit_graphics(gpu_context *gpu, VkCommandBuffer cmd);
void gpu_context_release(gpu_context *gpu);
void gpu_context_set_present_mode(gpu_context *gpu, VkPresentModeKHR present_mode);
b8 gpu_context_try_begin_frame(gpu_context *gpu, window *win, arena *temp_mem);
void gpu_context_end_frame(gpu_context *gpu, arena *temp_mem);
void gpu_context_immidiate_graphics(gpu_context *gpu);

u32 gpu_buffer_type_index(gpu_device *device, gpu_physical_device *pdevice, VkBufferUsageFlags usage, VkBufferCreateFlags flags, VkMemoryPropertyFlags properties);
u32 gpu_image_type_index(gpu_device *device, gpu_physical_device *pdevice, VkFormat format, VkSampleCountFlagBits samples, VkImageCreateFlags flags, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

#endif