#ifndef jahoda_glfw_context
#define jahoda_glfw_context

#include <jahoda/core/core.h>
#include "vk_glfw.h"

typedef struct
{
	bool8 initialized;
} glfw_context;

glfw_context glfw_context_make();
void glfw_context_release(glfw_context *ctx);

#endif