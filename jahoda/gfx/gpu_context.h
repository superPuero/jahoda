#ifndef jahoda_gpu_context
#define jahoda_gpu_context

#include <shaderc/shaderc.h>
#include "vk_instance.h"
#include "vk_physical_device.h"
#include "vk_surface.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "gpu_arena.h"
#include "render_target_manager.h"

#define jahoda_gpu_context_max_graphics_cmd_count 64

#define temp_host_coherent_buffer_arena_size Mb(16)
#define static_device_local_swapchain_image_arena_size Mb(32)

typedef struct
{
	vk_instance instance;
	vk_physical_device pdevice;
	vk_surface surface;
	vk_device device;	
	vk_swapchain swapchain;
	
	VkQueue graphics_queue;
	VkQueue transfer_queue;
	VkQueue present_queue;
	
	u32 frame_index;
	u32 swapchain_image_index;	
	bool8 swapchain_needs_refresh;
	bool8 swapchain_was_refreshed;
	
	VkCommandPool graphics_pool;
	VkCommandPool transfer_pool;
		
	VkSemaphore graphics_semaphores[jahoda_vk_max_swapchain_image_count];
	
	VkFence render_fences[jahoda_vk_frames_in_flight];
	u32 next_graphics_buffer[jahoda_vk_frames_in_flight];
	VkCommandBuffer graphics_buffers[jahoda_gpu_context_max_graphics_cmd_count][jahoda_vk_frames_in_flight];
	
	u32 next_transfer_buffer[jahoda_vk_frames_in_flight];
	VkCommandBuffer transfer_buffers[jahoda_gpu_context_max_graphics_cmd_count][jahoda_vk_frames_in_flight];
	
	u32 commited_graphics_count[jahoda_vk_frames_in_flight];
	VkCommandBuffer commited_graphics_buffers[jahoda_gpu_context_max_graphics_cmd_count][jahoda_vk_frames_in_flight];
	
	VkSemaphore image_avalible_semaphores[jahoda_vk_frames_in_flight];
	
	gpu_arena temp_hc_buf_arenas[jahoda_vk_frames_in_flight];
	gpu_arena static_dl_img_arenas[jahoda_vk_frames_in_flight]; 	
	gpu_arena static_dl_sw_img_arenas[jahoda_vk_frames_in_flight]; 	

} gpu_context;	

gpu_context gpu_context_make(arena *temp_arena, arena *static_arena, window *win);

VkCommandBuffer gpu_context_grab_graphics(gpu_context *gpu);
VkCommandBuffer gpu_context_grab_transfer(gpu_context *gpu);

void gpu_context_commit_graphics(gpu_context *gpu, VkCommandBuffer cmd);

void gpu_context_release(gpu_context *gpu);
bool gpu_context_try_begin_frame(gpu_context *ctx, window *win);
void gpu_context_end_frame(gpu_context *ctx);
void gpu_context_immidiate_graphics(gpu_context *gpu, VkCommandBuffer cmd);
void gpu_context_immidiate_transfer(gpu_context *gpu, VkCommandBuffer cmd);

#endif