#include "render_state.h"
#include "gpu_shader.h"

render_state render_state_make_(render_state_params params)
{
    render_state out = {0};
    
    out.push_constant_stages = params.push_constant_stages;

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
        .bindingCount = params.descriptor_binding_count,
        .pBindings = params.descriptor_bindings,		
    };

    gpu_check(vkCreateDescriptorSetLayout(params.gpu->device.handle, &info, NULL, &out.descriptor_layout));        
    
	VkPipelineLayoutCreateInfo light_pipeline_layout_ci = {
		.sType =  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,			
		.pushConstantRangeCount = params.push_constants_size ? 1 : 0,
		.pPushConstantRanges = (VkPushConstantRange[]){{
            .size = params.push_constants_size, 
            .offset = 0, 
            .stageFlags = params.push_constant_stages
        }},
		.setLayoutCount = 1,
		.pSetLayouts = &out.descriptor_layout
	};
	
	gpu_check(vkCreatePipelineLayout(params.gpu->device.handle, &light_pipeline_layout_ci, NULL, &out.pipeline_layout));

	VkPipelineDynamicStateCreateInfo dyn_state_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = params.dynamic_state_count,
		.pDynamicStates = params.dynamic_state
	};

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = NULL,
		.scissorCount = 1,
		.pScissors = NULL
	};

	shader_bytes vertex_bytes = shader_bytes_from_file(
		params.temp_arena, 
		params.vertex_shader_path
	);

	shader_bytes fragment_bytes = shader_bytes_from_file(
		params.temp_arena,
		params.fragment_shader_path
	);

	VkShaderModule vertex_shader = gpu_shader_module_from_shader_bytes(&params.gpu->device, &vertex_bytes);
	VkShaderModule fragment_shader = gpu_shader_module_from_shader_bytes(&params.gpu->device, &fragment_bytes);    
	
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_ci[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertex_shader,
			.pName = "main"
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragment_shader,
			.pName = "main"
		}
	};
	
    VkVertexInputBindingDescription *vertex_bindings =  arena_ppush_arr(params.temp_arena, VkVertexInputBindingDescription, params.vertex_binding_count);
    VkVertexInputAttributeDescription *vertex_attributes =  arena_ppush_arr(params.temp_arena, VkVertexInputAttributeDescription, params.vertex_binding_count);

    u32 binding = 0;

    for each_index(i, params.vertex_binding_count)
    {
        vertex_bindings[i].binding = i;
        vertex_bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_bindings[i].stride = params.vertex_bindings[i].stride;

        vertex_attributes[i].binding = i;
        vertex_attributes[i].format = params.vertex_bindings[i].format;
        vertex_attributes[i].location = params.vertex_bindings[i].location;
        vertex_attributes[i].offset = 0;
    }

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pVertexBindingDescriptions = vertex_bindings,
		.vertexBindingDescriptionCount = params.vertex_binding_count,
		.pVertexAttributeDescriptions = vertex_attributes,
		.vertexAttributeDescriptionCount = params.vertex_binding_count,
	};

    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = params.topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = params.polygon_mode,
        .cullMode = params.cull_mode,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };   

    VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = params.depth_test,
		.depthWriteEnable = params.depth_write,
		.depthCompareOp = params.depth_compare_op,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable= VK_FALSE,
		.front = {0},
		.back = {0},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};

	VkPipelineColorBlendAttachmentState color_blend_attachments[8] = {0};
    for(u32 i = 0; i < params.color_target_count; i++) {
        color_blend_attachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachments[i].blendEnable = VK_FALSE;
    }

	VkPipelineColorBlendStateCreateInfo color_blend_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = params.color_target_count,
		.pAttachments = color_blend_attachments,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
	};
	
	
    VkPipelineRenderingCreateInfo pipeline_rendering_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pColorAttachmentFormats = params.color_target_formats,
        .colorAttachmentCount = params.color_target_count,
        .depthAttachmentFormat = params.depth_target_format,
    };

	VkGraphicsPipelineCreateInfo pipeline_ci = { 
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &pipeline_rendering_ci,
		.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,		
		.stageCount = arrlen(pipeline_shader_stage_ci),
		.pStages = pipeline_shader_stage_ci,
		.pVertexInputState = &vertex_input_state,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisample,
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blend_state,
		.pDynamicState = &dyn_state_ci,
		.layout = out.pipeline_layout,		
		.renderPass = VK_NULL_HANDLE,		
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	gpu_check(vkCreateGraphicsPipelines(params.gpu->device.handle, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &out.pipeline));    

    vkDestroyShaderModule(params.gpu->device.handle, vertex_shader, NULL);
    vkDestroyShaderModule(params.gpu->device.handle, fragment_shader, NULL);

	params.gpu->device.get_descriptor_set_layout_size(params.gpu->device.handle, out.descriptor_layout, &out.descriptor_layout_size);

    return out;
}

void render_state_release(render_state *state, gpu_context *gpu)
{	
    vkDestroyPipelineLayout(gpu->device.handle, state->pipeline_layout, NULL);	
    vkDestroyPipeline(gpu->device.handle, state->pipeline, NULL);
    vkDestroyDescriptorSetLayout(gpu->device.handle, state->descriptor_layout, NULL);	
}