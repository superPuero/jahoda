#include "gpu_shader.h"

shader_bytes shader_bytes_from_file(arena *scratch_arena, str8 path)
{
	str8 data = file_load(scratch_arena, path);
	verify((data.size % 4) == 0, ".spv content len must be divisible by 4");
	
	u32 wide_len = data.size / 4;
	shader_bytes bytes = {0};

	da_resize(&bytes, scratch_arena, wide_len);
	memcpy(bytes.data, data.data, data.size);

	return bytes;
}

VkShaderModule gpu_shader_module_from_shader_bytes(gpu_device *device, shader_bytes *bytes)
{
	VkShaderModuleCreateInfo shader_module_ci = { 
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = bytes->occupied  *sizeof(u32),
		.pCode = bytes->data
	};

	VkShaderModule out = NULL;

	gpu_check(vkCreateShaderModule(device->handle, &shader_module_ci, NULL, &out));

	return out;
}

void gpu_shader_module_release(gpu_device *device, VkShaderModule module)
{
	vkDestroyShaderModule(device->handle, module, NULL);
}