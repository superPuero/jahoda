#include "compute_state.h"
#include "gpu_shader.h"

compute_state compute_state_make_(compute_state_params params)
{
    compute_state out = {0};

	shader_bytes comp_shader = shader_bytes_from_file(
		params.temp_mem,
		params.shader_path
	);

	VkShaderModule shader = gpu_shader_module_from_shader_bytes(&params.gpu->device, &comp_shader);

    VkPushConstantRange pc_range = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = params.push_constants_size
    };

    VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = params.push_constants_size > 0 ? 1 : 0,
        .pPushConstantRanges = params.push_constants_size > 0 ? &pc_range : NULL,
        .setLayoutCount = 0, 
        .pSetLayouts = NULL
    };

    gpu_check(vkCreatePipelineLayout(params.gpu->device.handle, &layout_info, NULL, &out.pipeline_layout));

    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = out.pipeline_layout,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader,
            .pName = "main"
        }
    };

    gpu_check(vkCreateComputePipelines(
        params.gpu->device.handle, 
        VK_NULL_HANDLE, 
        1, &pipeline_info, 
        NULL, 
        &out.pipeline));

    vkDestroyShaderModule(params.gpu->device.handle, shader, NULL);

    return out;
}

void compute_state_release(compute_state *state, gpu_context *gpu)
{
    vkDestroyPipelineLayout(gpu->device.handle, state->pipeline_layout, NULL);	
    vkDestroyPipeline(gpu->device.handle, state->pipeline, NULL);
    vkDestroyDescriptorSetLayout(gpu->device.handle, state->descriptor_layout, NULL);	
}
