#ifndef jahoda_vk
#define jahoda_vk

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <jahoda/core/utils.h>
#include <jahoda/core/da.h>

#define frames_in_flight 3
#define per_frame_in_flight [static frames_in_flight]

const char *gpu_result_to_string(VkResult res);

#define gpu_check(expr) \
do { \
	VkResult val = (expr);\
	dbg_verifyl(gpu_check_fail, val == VK_SUCCESS, "%s resulted in %s", #expr, gpu_result_to_string(val));\
} while(0)

#endif