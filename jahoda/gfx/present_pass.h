#ifndef jahoda_present_pass
#define jahoda_present_pass

#include "gpu_context.h"

typedef struct
{
	arena *pf_arena;

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkShaderModule vertex_shader;	
	VkShaderModule fragment_shader;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkSampler sampler;	
	VkFramebuffer framebuffers[jahoda_vk_max_swapchain_image_count];
	VkDescriptorSet descriptor_sets[jahoda_vk_frames_in_flight];	
	VkRenderPass render_pass;
} present_pass;

present_pass present_pass_make(arena *pf_arena, gpu_context *gpu, VkImageView input_views[jahoda_vk_frames_in_flight]);
void present_pass_release(gpu_context *gpu, present_pass *pass);

void present_pass_record(gpu_context *gpu, present_pass *pass,  VkImageView input_views[jahoda_vk_frames_in_flight], VkCommandBuffer cmd);

#endif