#include "gpu_arena.h"

gpu_arena gpu_arena_make(vk_device *device, VkDeviceSize capacity, u32 memory_type_index, strv name, bool8 map) 
{
	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = capacity,
		.memoryTypeIndex = memory_type_index
	};

    gpu_arena out = {
		.capacity = capacity,
		.current = 0,
		.memory_type_index = memory_type_index
	};

	verify(name.len < sizeof(out.name), "name.len < sizeof(out.name)");

	memcpy(out.name, name.data, name.len);

    vk_check(vkAllocateMemory(device->handle, &alloc_info, NULL, &out.mem));

	if(map)
	{
		vk_check(vkMapMemory(device->handle, out.mem, 0, out.capacity, 0, (void*)&out.mapped_ptr));
	}

	infol(gpu_alloc, "(%.*s) %.2lf Mb", strv_fmt(&name),  capacity / 1024.0 / 1024.0);

    return out;
}

VkDeviceSize gpu_arena_push(gpu_arena *arena, VkDeviceSize size, VkDeviceSize alignment) 
{
	VkDeviceSize aligned_offset = ((arena->current + alignment - 1) & ~(alignment - 1)) - arena->current;
	
    arena->current += aligned_offset;

	VkDeviceSize out = arena->current;

	arena->current += size;

	if(arena->current > arena->capacity)
	{
		printf("gpu_arena [%s] overflow\n", arena->name);
		dbg_verify(0, "");
	}
    
    return out;
}

gpu_marker gpu_arena_mark(gpu_arena *arena)
{
	return (gpu_marker){ .arena = arena, .point = arena->current };
}

void gpu_arena_reset(gpu_arena *arena)
{
	arena->current = 0;
}

void gpu_arena_release(vk_device *device, gpu_arena *arena)
{
	if(arena->mapped_ptr)
	{
		vkUnmapMemory(device->handle, arena->mem);
	}
	
	vkFreeMemory(device->handle, arena->mem, NULL);
}