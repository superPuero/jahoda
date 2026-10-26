#ifndef jahoda_gpu_arena
#define jahoda_gpu_arena

#include <jahoda/core/core.h>
#include <jahoda/gfx/gpu_device.h>
#include <jahoda/gfx/gpu_glfw.h>
#include <jahoda/gfx/gpu_texture.h>

#define gpu_buffer_arena_max_buffers 64
#define gpu_texture_arena_max_textures 64

typedef struct
{
    VkBuffer handle;
    VkDeviceAddress device_address;
    void *mem;
    u64 size;
    text128 tag;
} gpu_buffer;

typedef struct 
{
    VkBufferUsageFlags usage;
    VkSharingMode sharing_mode;
    VkBufferCreateFlags flags;

    VkDeviceMemory mem;
    VkDeviceSize capacity;
    VkDeviceSize current;    
	u8 *map; 
	text128 name;

    u32 buffers_capacity;
    u32 buffers_occupied;
    gpu_buffer buffers[gpu_buffer_arena_max_buffers];
} gpu_buffer_arena;

typedef struct
{
    gpu_device *device;
    arena *mem;
    VkDeviceSize capacity;
    VkBufferUsageFlags usage;
    VkSharingMode sharing_mode;
    VkBufferCreateFlags flags;
    VkMemoryPropertyFlags properties;
    text128 name;
    const char *ccname;
} gpu_buffer_arena_params;
gpu_buffer_arena *gpu_buffer_arena_make_(gpu_buffer_arena_params params);
#define gpu_buffer_arena_make(...) gpu_buffer_arena_make_((gpu_buffer_arena_params){ .sharing_mode = VK_SHARING_MODE_EXCLUSIVE, __VA_ARGS__})
void gpu_buffer_arena_reset(gpu_buffer_arena *arena, gpu_device *device);
void gpu_buffer_arena_release(gpu_buffer_arena *arena, gpu_device *device);

VkDeviceSize gpu_buffer_arena_push_bytes(gpu_buffer_arena *arena, VkDeviceSize size, VkDeviceSize alignment);

typedef struct
{
    gpu_buffer_arena *arena;
    u32 current;
    VkDeviceSize mem_current;
} gpu_buffer_arena_scratch;

gpu_buffer_arena_scratch gpu_buffer_arena_scratch_make(gpu_buffer_arena *arena);
void gpu_buffer_arena_scratch_pop(gpu_buffer_arena_scratch scratch, gpu_device *device);

typedef struct
{
    gpu_buffer_arena *arena;
    text128 tag;
    gpu_device *device;
    u64 size;
    type_info type_info;
} gpu_buffer_arena_push_params;

gpu_buffer *gpu_buffer_arena_push_(gpu_buffer_arena_push_params params);
#define gpu_buffer_arena_push(type, ...) gpu_buffer_arena_push_((gpu_buffer_arena_push_params){.type_info = type_info_of(type), __VA_ARGS__})

typedef u64 texture_id;

typedef struct 
{
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageCreateFlags flags;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkSharingMode sharing_mode;

    VkDeviceMemory mem;
    VkDeviceSize capacity;
    VkDeviceSize current;
	u8 *map; 
	text128 name;

    u32 textures_occupied;
    gpu_texture textures[gpu_texture_arena_max_textures];
} gpu_texture_arena;

typedef struct 
{
    gpu_device *device;
    arena *mem;
    VkDeviceSize capacity;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageCreateFlags flags;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags properties;
    VkSharingMode sharing_mode;
    text128 name;
} gpu_texture_arena_params;

gpu_texture_arena *gpu_texture_arena_make_(gpu_texture_arena_params params);
#define gpu_texture_arena_make(...) gpu_texture_arena_make_((gpu_texture_arena_params){__VA_ARGS__})

VkDeviceSize gpu_texture_arena_push_bytes(gpu_texture_arena *arena, VkDeviceSize size, VkDeviceSize alignment);

typedef struct
{
    gpu_texture_arena *arena;
    text128 tag;
    gpu_device *device;
    VkExtent3D extent;
    u32 mip_levels;
    u32 array_layers;
    VkImageLayout initial_layout;
} gpu_texture_arena_push_params;

gpu_texture *gpu_texture_arena_push_(gpu_texture_arena_push_params params);
#define gpu_texture_arena_push(...) gpu_texture_arena_push_((gpu_texture_arena_push_params){\
    .mip_levels = 1,\
    .array_layers = 1,\
    .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,\
    __VA_ARGS__\
})

void gpu_texture_arena_reset(gpu_texture_arena *arena, gpu_device *device);
void gpu_texture_arena_release(gpu_texture_arena *arena, gpu_device *device);

#endif