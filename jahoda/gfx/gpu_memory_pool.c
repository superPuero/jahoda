// #include "gpu_memory_pool.h"
    
// gpu_buffer_arena *gpu_memory_pool_add_buffer_arena(gpu_memory_pool *pool, gpu_device *device, VkDeviceSize capacity, VkBufferUsageFlags usage, VkBufferCreateFlags flags, VkMemoryPropertyFlags properties, strv name)
// {
//     pool->buffer_arenas[pool->current_buffer_arena++] = gpu_buffer_arena_make(device, capacity, usage, flags, properties, name);
//     return pool->buffer_arenas + pool->current_buffer_arena++;
// }
// gpu_texture_arena *gpu_memory_pool_add_texture_arena(gpu_memory_pool *pool, gpu_device *device, VkDeviceSize capacity, VkFormat format, VkSampleCountFlagBits samples, VkImageCreateFlags flags, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, strv name)
// {
//     pool->texture_arenas[pool->current_texture_arena] = gpu_texture_arena_make(device, capacity, format, samples, flags, tiling, usage, properties, name);
//     return pool->texture_arenas + pool->current_texture_arena++;
// }
// void gpu_memory_pool_relase(gpu_device *device, gpu_memory_pool *pool)
// {
//     for each_index(i, pool->current_buffer_arena)
//     {
//         gpu_buffer_arena_release(device, pool->buffer_arenas + i);
//     }
//     for each_index(i, pool->current_texture_arena)
//     {
//         gpu_texture_arena_release(pool->texture_arenas + i, device);
//     }
// }
// void gpu_memory_poo_reset(gpu_memory_pool *pool)
// {
    
// }