#include "vk_shader.h"

shader_bytes shader_bytes_from_file(arena *scratch_arena, strv path)
{
	str data = file_load(scratch_arena, path);
	verify((data.occupied % 4) == 0, ".spv content len must be divisible by 4");
	
	u32 wide_len = data.occupied / 4;
	shader_bytes bytes = {0};

	da_resize(scratch_arena, &bytes, wide_len);
	memcpy(bytes.data, data.data, data.occupied);

	return bytes;
}

VkShaderModule vk_shader_module_from_shader_bytes(vk_device *device, shader_bytes *bytes)
{
	VkShaderModuleCreateInfo shader_module_ci = { 
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = bytes->occupied  *sizeof(u32),
		.pCode = bytes->data
	};

	VkShaderModule out = NULL;

	vk_check(vkCreateShaderModule(device->handle, &shader_module_ci, NULL, &out));

	return out;
}

void vk_shader_module_release(vk_device *device, VkShaderModule module)
{
	vkDestroyShaderModule(device->handle, module, NULL);
}