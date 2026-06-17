// #ifndef vk_texture_array
// #define vk_texture_array

// #include "gpu_context.h"
// #include "gpu_arena.h"
// #include "texture.h"

// da_declare(texture, texture_da);

// typedef struct
// {
//     gpu_arena dl_st_arena;    
//     arena **st_arena;
    
//     VkDescriptorPool pool;
//     VkDescriptorSetLayout descriptor_set_layout;
//     VkDescriptorSet descriptor_set;

//     texture_da entries;

// } vk_texture_array;

// vk_texture_array texture_array_create(gpu_context *gpu, gpu_arena *dl_st_arena, arena **st_arena, u32 dl_memory_size, u32 max_entries);
// u32 texture_arena_record_upload(vk_texture_array *texture_array, image *image, gpu_arena *hc_pf_array, VkCommandBuffer cmd);

// #endif