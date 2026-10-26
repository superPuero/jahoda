#include "gpu_arena.h"

internal VkMemoryRequirements2 buffer_memory_requirements(gpu_device *device, VkBufferUsageFlags usage, VkSharingMode sharing_mode, VkBufferCreateFlags flags, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo buffer_ci = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = 1,
			.usage = usage,
			.flags = flags, 
			.sharingMode = sharing_mode
		};

	VkDeviceBufferMemoryRequirements buffer_req_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
		.pCreateInfo = &buffer_ci
	};

	VkMemoryRequirements2 mem_reqs = { .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
	vkGetDeviceBufferMemoryRequirements(device->handle, &buffer_req_info, &mem_reqs);

	return mem_reqs;
}

internal VkMemoryRequirements2 image_memory_requirements(gpu_device *device, VkFormat format, VkSampleCountFlagBits samples, VkImageCreateFlags flags, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharing_mode, VkMemoryPropertyFlags properties)
{
	VkImageCreateInfo image_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.flags = flags,
		.format = format, 
		.extent = {1, 1, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = samples,
		.tiling = tiling,
		.usage = usage, 
		.sharingMode = sharing_mode,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkDeviceImageMemoryRequirements image_req_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
		.pCreateInfo = &image_ci
	};

	VkMemoryRequirements2 mem_reqs = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
	};
	vkGetDeviceImageMemoryRequirements(device->handle, &image_req_info, &mem_reqs);

	return mem_reqs;
}

gpu_buffer_arena *gpu_buffer_arena_make_(gpu_buffer_arena_params params)
{
	VkMemoryRequirements2 requirements = buffer_memory_requirements(params.device, params.usage, params.sharing_mode, params.flags, params.properties);

	gpu_buffer_arena *out = arena_ppush(params.mem, gpu_buffer_arena);

	*out = (gpu_buffer_arena){
		.capacity = params.capacity,
		.current = 0,
		.usage = params.usage,
		.flags = params.flags,
		.name = params.name,
		.sharing_mode = params.sharing_mode
	};

	VkMemoryAllocateFlagsInfo alloc_flags = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.pNext = NULL,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT // | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT
	};

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = params.capacity,
		.pNext = &alloc_flags,
		.memoryTypeIndex = gpu_physical_device_pick_memory_type(
			params.device->pdevice, 
			requirements.memoryRequirements.memoryTypeBits,            
			params.properties
		)
	};	
 
    gpu_check(vkAllocateMemory(params.device->handle, &alloc_info, NULL, &out->mem));

	if(params.properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		info("%s", params.name.data);
		gpu_check(vkMapMemory(params.device->handle, out->mem, 0, out->capacity, 0, (void*)&out->map));
	}

	infol(gpu_mem, "[%s]" " %.2lf Mb", out->name.data, as_Mb(out->capacity));

    return out;
}

VkDeviceSize gpu_buffer_arena_push_bytes(gpu_buffer_arena *arena, VkDeviceSize size, VkDeviceSize alignment) 
{
	VkDeviceSize aligned_offset = (arena->current + alignment - 1) & ~(alignment - 1);
	

	arena->current = aligned_offset + size;
	
	dbg_verify(arena->current <= arena->capacity, "[%s] overflow\n", arena->name.data);

    return aligned_offset;
}

gpu_buffer *gpu_buffer_arena_push_(gpu_buffer_arena_push_params params)
{
	dbg_verifyl(gpu_mem, params.arena->buffers_occupied < gpu_buffer_arena_max_buffers, 
		"[%s] buffer limit [%u] was reached", params.arena->name.data, gpu_buffer_arena_max_buffers);

    gpu_buffer *out = params.arena->buffers + params.arena->buffers_occupied++;
    
	out->size = params.size * params.type_info.size;
	out->tag = params.tag;
    VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = params.size * params.type_info.size,
        .usage = params.arena->usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .flags = params.arena->flags, // VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT
        .sharingMode = params.arena->sharing_mode
    };
    
    gpu_check(vkCreateBuffer(params.device->handle, &buffer_ci, NULL, &out->handle));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(params.device->handle, out->handle, &mem_reqs);
    
    VkDeviceSize final_alignment = mem_reqs.alignment > params.type_info.alignment ? mem_reqs.alignment : params.type_info.alignment;
    
	if(params.arena->usage & VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT)
	{
		final_alignment = final_alignment > params.device->pdevice->descriptor_buffer_properties.descriptorBufferOffsetAlignment ? final_alignment : params.device->pdevice->descriptor_buffer_properties.descriptorBufferOffsetAlignment;
	}

    VkDeviceSize alloc = gpu_buffer_arena_push_bytes(params.arena, mem_reqs.size, final_alignment);
    uz bytes = params.arena->current - alloc;
    
    gpu_check(vkBindBufferMemory(params.device->handle, out->handle, params.arena->mem, alloc));
    
    if(params.arena->map)
    {
        out->mem = (void*)(params.arena->map + alloc);
    }
    
    
    VkBufferDeviceAddressInfo desc_addr_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = out->handle
    };
    
    out->device_address = vkGetBufferDeviceAddress(params.device->handle, &desc_addr_info);
    
	if(as_Kb(bytes) > 1000.0)
	{
		infol(gpu_mem, "[%s] pushed %.2lf Mb [%s] %.2lf Mb left", 
			  params.arena->name.data, as_Mb(bytes), params.tag.data, 
			  as_Mb(params.arena->capacity - params.arena->current));
	}
	else if(as_Kb(bytes) > 1)
	{
		infol(gpu_mem, "[%s] pushed %.2lf Kb [%s] %.2lf Mb left", 
			  params.arena->name.data, as_Kb(bytes), params.tag.data, 
			  as_Mb(params.arena->capacity - params.arena->current));	
	}
	else
	{
		infol(gpu_mem, "[%s] pushed %llu bytes [%s] %.2lf Mb left", 
			  params.arena->name.data, bytes, params.tag.data, 
			  as_Mb(params.arena->capacity - params.arena->current));		
	}
          
    return out;
}

void gpu_buffer_arena_reset(gpu_buffer_arena *arena, gpu_device *device)
{
	u32 count = arena->buffers_occupied;
	for each_index(i, arena->buffers_occupied)
	{		
		vkDestroyBuffer(device->handle, arena->buffers[i].handle, NULL);
	}

	arena->buffers_occupied = 0;	
	if(arena->current != 0)
	{		
		infol(gpu_mem, "[%s] reset %u buffers (%.2lf Mb), back to %.2lf Mb", arena->name.data, count, as_Mb(arena->current), as_Mb(arena->capacity));\
	}
	arena->current = 0;
}

void gpu_buffer_arena_release(gpu_buffer_arena *arena, gpu_device *device)
{
	gpu_buffer_arena_reset(arena, device);

	if(arena->map)
	{	
		vkUnmapMemory(device->handle, arena->mem);
	}
	
	vkFreeMemory(device->handle, arena->mem, NULL);
}

gpu_texture_arena *gpu_texture_arena_make_(gpu_texture_arena_params params)
{
	VkMemoryRequirements2 requirements = image_memory_requirements(
		params.device, 
		params.format, 
		params.samples, 
		params.flags, 
		params.tiling, 
		params.usage, 
		params.sharing_mode,
		params.properties
	);

	gpu_texture_arena *out = arena_ppush(params.mem, gpu_texture_arena);
	*out = (gpu_texture_arena){
		.textures_occupied = 0,
		.capacity = params.capacity,
		.current = 0,
		.usage = params.usage,
		.format = params.format,
		.samples = params.samples,
		.flags = params.flags,
		.tiling = params.tiling,
		.sharing_mode = params.sharing_mode,
		.name = params.name
	};

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = out->capacity,
		.memoryTypeIndex = gpu_physical_device_pick_memory_type(
			params.device->pdevice, 
			requirements.memoryRequirements.memoryTypeBits, 
			params.properties
		)
	};
	
    gpu_check(vkAllocateMemory(params.device->handle, &alloc_info, NULL, &out->mem));

	if(params.properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		gpu_check(vkMapMemory(params.device->handle, out->mem, 0, out->capacity, 0, (void*)&out->map));
	}

	infol(gpu_mem,  "[%s]" " %.2lf Mb", out->name.data, as_Mb(out->capacity));

    return out;
}

VkDeviceSize gpu_texture_arena_push_bytes(gpu_texture_arena *arena, VkDeviceSize size, VkDeviceSize alignment) 
{
	VkDeviceSize aligned_offset = (arena->current + alignment - 1) & ~(alignment - 1);
	
	arena->current = aligned_offset + size;

	dbg_verify(arena->current <= arena->capacity, "[%s] overflow, arena->current = %zu, arena->capacity = %zu\n", arena->name.data, arena->current, arena->capacity);
	
    return aligned_offset;
}

gpu_texture *gpu_texture_arena_push_(gpu_texture_arena_push_params params) 
{
	dbg_verifyl(gpu_mem, params.arena->textures_occupied < gpu_texture_arena_max_textures, "[%s] texture limit [%u] was reached", params.arena->name.data, gpu_texture_arena_max_textures);

	gpu_texture *out = params.arena->textures + params.arena->textures_occupied++; 
	out->format = params.arena->format;
	out->extent = params.extent;
	out->tag = params.tag;
	out->state = gpu_texture_state_undefined;

	VkImageCreateInfo image_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = params.arena->format,
		.extent = params.extent,
		.mipLevels = params.mip_levels,
		.arrayLayers = params.array_layers,
		.samples = params.arena->samples,
		.tiling = params.arena->tiling,
		.usage = params.arena->usage,
		.sharingMode = params.arena->sharing_mode,
		.initialLayout = params.initial_layout,
	};

	gpu_check(vkCreateImage(params.device->handle, &image_ci, NULL, &out->handle));
	VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(params.device->handle, out->handle, &mem_reqs);
	
	VkDeviceSize alloc = gpu_texture_arena_push_bytes(
		params.arena, 
		mem_reqs.size, 
		mem_reqs.alignment
	);

	uz bytes = params.arena->current - alloc;

	gpu_check(vkBindImageMemory(params.device->handle, out->handle, params.arena->mem, alloc));	
	if(params.arena->map)
	{
		out->mem = params.arena->map + alloc;
	}
	
	VkImageViewCreateInfo view_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = out->handle,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = params.arena->format,
		.subresourceRange = {
			.aspectMask = params.arena->format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

    gpu_check(vkCreateImageView(params.device->handle, &view_ci, NULL, &out->view_handle));

	if(as_Kb(bytes) > 1000.0)
	{
		infol(gpu_mem, "[%s] pushed %.2lf Mb [%s] %.2lf Mb left", 
			  params.arena->name.data, as_Mb(bytes), params.tag.data, 
			  as_Mb(params.arena->capacity - params.arena->current));
	}
	else if(as_Kb(bytes) > 1)
	{
		infol(gpu_mem, "[%s] pushed %.2lf Kb [%s] %.2lf Mb left", 
			  params.arena->name.data, as_Kb(bytes), params.tag.data, 
			  as_Mb(params.arena->capacity - params.arena->current));	
	}
	else
	{
		infol(gpu_mem, "[%s] pushed %llu bytes [%s] %.2lf Mb left", 
			  params.arena->name.data, bytes, params.tag.data, 
			  as_Mb(params.arena->capacity - params.arena->current));		
	}

	return out;
}

void gpu_texture_arena_reset(gpu_texture_arena *arena, gpu_device *device)
{
	u32 count = arena->textures_occupied; 
	for each_index(i, arena->textures_occupied)
	{		
		vkDestroyImageView(device->handle, arena->textures[i].view_handle, NULL);
		vkDestroyImage(device->handle, arena->textures[i].handle, NULL);
	}

	arena->textures_occupied = 0;
		
	if(arena->current != 0)
	{		
		infol(gpu_mem, "[%s] popped %u textures (%.2lf Mb), back to %.2lf Mb", arena->name.data, count, as_Mb(arena->current), as_Mb(arena->capacity));\
	}

	arena->current = 0;
}

void gpu_texture_arena_release(gpu_texture_arena *arena, gpu_device *device)
{
	gpu_texture_arena_reset(arena, device);

	if(arena->map)
	{
		vkUnmapMemory(device->handle, arena->mem);
	}
	
	vkFreeMemory(device->handle, arena->mem, NULL);
}

gpu_buffer_arena_scratch gpu_buffer_arena_scratch_make(gpu_buffer_arena *arena)
{
	return (gpu_buffer_arena_scratch){.arena = arena, .current = arena->buffers_occupied, .mem_current = arena->current};
}
void gpu_buffer_arena_scratch_pop(gpu_buffer_arena_scratch scratch, gpu_device *device)
{
	for each_index_range(i, scratch.current, scratch.arena->buffers_occupied)
	{		
		vkDestroyBuffer(device->handle, scratch.arena->buffers[i].handle, NULL);
	}

	scratch.arena->buffers_occupied = scratch.current;		
	scratch.arena->current = scratch.mem_current;	
}
