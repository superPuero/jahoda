#include "present_pass.h"
#include "vk_shader.h"

present_pass present_pass_make(arena *pf_arena, gpu_context* gpu, VkImageView input_views[jahoda_vk_frames_in_flight])
{
	strv vertex_shader_path = strv_from_cstr("shaders/quad.vert.spv");
	strv fragment_shader_path = strv_from_cstr("shaders/quad.frag.spv");;

	present_pass out = {0};
	out.pf_arena = pf_arena;

	// render_pass
	VkAttachmentDescription color_attachment = {
		.format = gpu->swapchain.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};
	
	VkRenderPassCreateInfo present_render_pass_ci = { 
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, 
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
		.flags = 0,
	};
	vk_check(vkCreateRenderPass(gpu->device.handle, &present_render_pass_ci, NULL, &out.render_pass));
	// ---------------



	// sampler
	VkSamplerCreateInfo sampler_ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR, 
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
    };
    vk_check(vkCreateSampler(gpu->device.handle, &sampler_ci, NULL, &out.sampler));
	// ---------------



	// framebuffers
	for(uz i = 0; i < gpu->swapchain.image_count; i++)
	{
		VkFramebufferCreateInfo framebuffer_ci = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = out.render_pass, 
			.attachmentCount = 1,            
			.pAttachments = &gpu->swapchain.image_views[i], 
			.width = gpu->swapchain.extent.width,
			.height = gpu->swapchain.extent.height,
			.layers = 1,
		};
	
		vk_check(vkCreateFramebuffer(gpu->device.handle, &framebuffer_ci, NULL, &out.framebuffers[i]));
	}
	// ---------------

	// descriptor pool
	VkDescriptorPoolSize descriptor_pool_size = {
		.descriptorCount = 3,
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
	};

	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = arrsize(out.descriptor_sets),
		.poolSizeCount = 1,
		.pPoolSizes = &descriptor_pool_size,
	};

	vk_check(vkCreateDescriptorPool(gpu->device.handle, &pool_info, NULL, &out.descriptor_pool));
	// ---------------



	// descriptor set layout
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
		.binding = 0,
		.descriptorType =VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = 1,
		.pBindingFlags = &binding_flags,
	};

	VkDescriptorSetLayoutCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &binding_flags_info,
		.bindingCount = 1,
		.pBindings = &descriptor_set_layout_binding,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
	};

	vk_check(vkCreateDescriptorSetLayout(gpu->device.handle, &info, NULL, &out.descriptor_set_layout));
	// ---------------

	// descriptor sets
	VkDescriptorSetLayout layouts[jahoda_vk_frames_in_flight];
    for (u32 i = 0; i < jahoda_vk_frames_in_flight; i++) 
	{
        layouts[i] = out.descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = out.descriptor_pool,
        .descriptorSetCount = jahoda_vk_frames_in_flight,
        .pSetLayouts = layouts
    };

    vk_check(vkAllocateDescriptorSets(gpu->device.handle, &alloc_info, out.descriptor_sets));
	// ----------------


	// descriptor set writes
	for (u32 i = 0; i < jahoda_vk_frames_in_flight; i++) 
	{     
        VkDescriptorImageInfo image_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = input_views[i],
            .sampler = out.sampler
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = out.descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &image_info
        };

        // This pushes the update to the GPU
        vkUpdateDescriptorSets(gpu->device.handle, 1, &write, 0, NULL);
    }
	// -------------- 

	// pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_ci = {
		.sType =  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &out.descriptor_set_layout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};

	vk_check(vkCreatePipelineLayout(gpu->device.handle, &pipeline_layout_ci, NULL, &out.pipeline_layout));
	// ---------------

	// pipeline
	VkDynamicState dyn_state[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dyn_state_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dyn_state
	};

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = NULL,
		.scissorCount = 1,
		.pScissors = NULL
	};

	shader_bytes vertex_bytes = shader_bytes_from_file(
		out.pf_arena, 
		vertex_shader_path
	);

	shader_bytes fragment_bytes = shader_bytes_from_file(
		out.pf_arena,
		fragment_shader_path
	);

	out.vertex_shader = vk_shader_module_from_shader_bytes(&gpu->device, &vertex_bytes);
	out.fragment_shader = vk_shader_module_from_shader_bytes(&gpu->device, &fragment_bytes);
	
	str vertex_shader_name_nt = str_from_view_nt(out.pf_arena, vertex_shader_path);
	str fragment_shader_name_nt = str_from_view_nt(out.pf_arena, fragment_shader_path);

	VkPipelineShaderStageCreateInfo pipeline_shader_stage_ci[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = out.vertex_shader,
			.pName = "main"
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = out.fragment_shader,
			.pName = "main"
		}
	};
		
	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL,
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
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
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable= VK_FALSE,
		.front = {0},
		.back = {0},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 0.0f,
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE, 
    };

	VkPipelineColorBlendStateCreateInfo color_blend_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
	};
	
	VkGraphicsPipelineCreateInfo pipeline_ci = { 
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = arrsize(pipeline_shader_stage_ci),
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
		.renderPass = out.render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
		.pNext = NULL,
	};

	vk_check(vkCreateGraphicsPipelines(gpu->device.handle, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &out.pipeline));
	// ---------------	

	return out;
}

void present_pass_release(gpu_context* gpu, present_pass* pass)
{
	vkDestroyPipeline(gpu->device.handle, pass->pipeline, NULL);
	vkDestroyPipelineLayout(gpu->device.handle, pass->pipeline_layout, NULL);
	vkDestroyDescriptorPool(gpu->device.handle, pass->descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(gpu->device.handle, pass->descriptor_set_layout, NULL);

	vkDestroyRenderPass(gpu->device.handle, pass->render_pass,  NULL);

	vkDestroySampler(gpu->device.handle, pass->sampler, NULL);

	vkDestroyShaderModule(gpu->device.handle, pass->vertex_shader, NULL);
	vkDestroyShaderModule(gpu->device.handle, pass->fragment_shader, NULL);

	// framebuffers
	for(uz i = 0; i < gpu->swapchain.image_count; i++)
	{
		vkDestroyFramebuffer(gpu->device.handle, pass->framebuffers[i], NULL);
	}	
}

void present_pass_record(gpu_context* gpu, present_pass* pass, VkImageView input_views[jahoda_vk_frames_in_flight], VkCommandBuffer cmd)
{		
	if(gpu->swapchain_was_refreshed)
	{
		arena *pf_arena = pass->pf_arena;
		present_pass_release(gpu, pass);
		*pass = present_pass_make(pf_arena, gpu, input_views);
	}

	VkClearValue clear_value = {
		.color = { 1, 0, 0, 1 }
	};

	VkRenderPassBeginInfo rp_begin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.framebuffer = pass->framebuffers[gpu->swapchain_image_index],
		.clearValueCount = 1,
		.pClearValues = &clear_value,
		.renderPass  = pass->render_pass,
		.renderArea = { 
			.offset = {0,0}, 
			.extent =  gpu->swapchain.extent
		} 
	};
	vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = gpu->swapchain.extent.width,
        .height = gpu->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = gpu->swapchain.extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->pipeline);

	vkCmdBindDescriptorSets(
		cmd, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		pass->pipeline_layout, 
		0,                                  
		1,                                  
		&pass->descriptor_sets[gpu->frame_index],
		0,                                  
		NULL                                
	);

	vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);	
}