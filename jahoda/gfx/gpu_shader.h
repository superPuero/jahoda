#ifndef jahoda_gpu_shader
#define jahoda_gpu_shader

#include "gpu_device.h"

da_declare(u32, shader_bytes);

typedef struct
{
    VkShaderEXT handle;
    VkDescriptorSetLayout descriptor_layout;
    
    str8 path;
} gpu_shader;

shader_bytes shader_bytes_from_file(arena *scratch_arena, str8 path);
VkShaderModule gpu_shader_module_from_shader_bytes(gpu_device *device, shader_bytes *bytes);
void gpu_shader_module_release(gpu_device *device, VkShaderModule module);

gpu_shader gpu_shader_make(gpu_device *device, arena *mem, arena *temp_mem, str8 path);
void gpu_shader_release(gpu_shader *shader, gpu_device *device);

#endif