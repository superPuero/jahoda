#ifndef jahoda_gpu_instance
#define jahoda_gpu_instance

#include "gpu_glfw.h"
#include <jahoda/core/str.h>

#define gpu_instance_debug_layer_list\
	X(VK_LAYER_KHRONOS_validation)\
	X(VK_LAYER_KHRONOS_synchronization2)

#define gpu_instance_debug_extension_list\
	X(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)

typedef struct
{
	VkInstance handle;
	VkDebugUtilsMessengerEXT debug_messenger;
} gpu_instance;

gpu_instance gpu_instance_make(arena *temp_arena);
void gpu_instance_release(gpu_instance *instance);

#endif