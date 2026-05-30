#ifndef jahoda_vk_shader
#define jahoda_vk_shader

#include "vk_device.h"

da_declare(u32, shader_bytes);

shader_bytes shader_bytes_from_file(arena *scratch_arena, strv path);
VkShaderModule vk_shader_module_from_shader_bytes(vk_device *device, shader_bytes *bytes);
void vk_shader_module_release(vk_device *device, VkShaderModule module);

#endif