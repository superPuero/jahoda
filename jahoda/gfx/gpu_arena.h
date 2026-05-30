#ifndef jahoda_gpu_arena
#define jahoda_gpu_arena

#include "vk_glfw.h"
#include "vk_device.h"

typedef struct 
{
    VkDeviceMemory mem;
    VkDeviceSize capacity;
    VkDeviceSize current;
    u32 memory_type_index;
	u8 *mapped_ptr; 
	char name[arena_name_max_len];
} gpu_arena;

typedef struct
{
	gpu_arena *arena;
	uz point;
} gpu_marker;


gpu_arena gpu_arena_make(vk_device *device, VkDeviceSize capacity, u32 memory_type_index, strv name, bool8 map);
gpu_marker gpu_arena_mark(gpu_arena *arena);

#define gpu_arena_ppush(arena, type) gpu_arena_push(arena, sizeof(type), jahoda_alignof(type)); 
VkDeviceSize gpu_arena_push(gpu_arena *arena, VkDeviceSize size, VkDeviceSize alignment);

void gpu_arena_reset(gpu_arena *arena);
void gpu_arena_release(vk_device *device, gpu_arena *arena);

#endif