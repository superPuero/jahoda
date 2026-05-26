#ifndef jahoda_vk
#define jahoda_vk

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <src/core/utils.h>
#include <src/core/da.h>

#define jahoda_vk_frames_in_flight 3
#define jahoda_vk_max_swapchain_image_count 8

const char* vk_result_to_string(VkResult res);

#define vk_check(expr) \
do { \
	VkResult val = (expr); \
	dbg_verifyl(vk_check_fail, val == VK_SUCCESS, "%s resulted in %s", #expr, vk_result_to_string(val));\
} while(0)

#endif