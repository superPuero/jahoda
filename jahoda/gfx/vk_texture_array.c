// #include "vk_texture_array.h"

// vk_texture_array texture_array_create(gpu_context *gpu,  arena *st_arena,  u32 dl_memory_size, u32 max_entries)
// {
//     vk_texture_array out = {0};

//     VkDescriptorPoolSize descirptor_pool_size = {
//         .descriptorCount = max_entries,
//         .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
//     };

//     VkDescriptorPoolCreateInfo descriptor_pool_ci = {
//         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//         .maxSets = max_entries,
//         .pPoolSizes = &descirptor_pool_size,
//         .poolSizeCount = 1,
//         .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT
//     };

//     vk_check(vkCreateDescriptorPool(gpu->device.handle, &descriptor_Pool_ci, NULL, out.descriptor_pool));
    
//     VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
//         .binding = 0,
//         .descriptorCount = 1024,
//         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
//     };

//     VkDescriptorBindingFlags descriptor_binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT; 

//     VkDescriptorSetLayoutBindingFlagsCreateInfo desciptor_set_layout_flags_ci = {
//         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
//         .bindingCount = 1,
//         .flags = &descriptor_binding_flags
//     };

//     VkDescriptorSetLayoutCreateInfo descriptor_set_layout = {
//         .sType = VK_STRUCTURE_TYPE_VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//         .pBindings = &descriptor_set_layout_binding,
//         .pNext = &desciptor_set_layout_flags_ci,
//         .bindingCount = 1,
//         .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT 
//     };

//     vk_check(vkCreateDescriptorSetLayout(gpu->device.handle, &descriptor_set_layout, NULL, &out.descriptor_set_layout));

//     out.dl_st_arena = gpu_arena_make(&gpu->device, dl_memory_size, );
//     return out;
// }

// texture texture_arena_record_upload(vk_texture_array *texture_array, image *image, gpu_arena *hc_pf_array, VkCommandBuffer cmd)
// {

//     da_append(texture_array->entries)
// }
