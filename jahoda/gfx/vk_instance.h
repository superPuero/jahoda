#ifndef jahoda_vk_instance
#define jahoda_vk_instance

#include "vk_glfw.h"
#include <jahoda/core/str.h>

#define vk_instance_debug_layer_list\
	X(VK_LAYER_KHRONOS_validation)\
	X(VK_LAYER_KHRONOS_synchronization2)

#define vk_instance_debug_extension_list\
	X(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)

typedef struct
{
	VkInstance handle;
	VkDebugUtilsMessengerEXT debug_messenger;
} vk_instance;

vk_instance vk_instance_make(arena *temp_arena);
void vk_instance_release(vk_instance *instance);

#endif