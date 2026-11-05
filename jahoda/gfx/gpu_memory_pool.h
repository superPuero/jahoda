// #ifndef jahoda_gpu_memory_pool
// #define jahoda_gpu_memory_pool

// #include "gpu_arena.h"

// #define gpu_memory_pool_max_buffer_arenas 32
// #define gpu_memory_pool_max_texture_arenas 32

// typedef struct
// {
//     gpu_buffer_arena buffer_arenas[gpu_memory_pool_max_buffer_arenas];
//     u32 current_buffer_arena;
//     gpu_texture_arena texture_arenas[gpu_memory_pool_max_texture_arenas];
//     u32 current_texture_arena;
// } gpu_memory_pool;

// gpu_buffer_arena *gpu_memory_pool_add_buffer_arena(gpu_memory_pool *pool, gpu_device *device, VkDeviceSize capacity, VkBufferUsageFlags usage, VkBufferCreateFlags flags, VkMemoryPropertyFlags properties, strv name);
// gpu_texture_arena *gpu_memory_pool_add_texture_arena(gpu_memory_pool *pool, gpu_device *device, VkDeviceSize capacity, VkFormat format, VkSampleCountFlagBits samples, VkImageCreateFlags flags, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, strv name);
// gpu_buffer gpu_memory_pool_alloc_buffer();
// gpu_texture gpu_memory_pool_alloc_texture();
// void gpu_memory_pool_relase(gpu_device *device, gpu_memory_pool *pool);
// void gpu_memory_poo_reset(gpu_memory_pool *pool);

// #endif