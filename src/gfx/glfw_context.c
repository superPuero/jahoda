#include "glfw_context.h"

glfw_context glfw_context_make()
{
	glfw_context ctx = {0};
	ctx.initialized = glfwInit();

	if(!ctx.initialized)
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
    }

	if(!glfwVulkanSupported())
    {
        fprintf(stderr, "Vulkan is not supported on this machine\n");
        glfw_context_release(false);
    }

	return ctx;
}

void glfw_context_release(glfw_context* ctx)
{
	glfwTerminate();
	ctx->initialized = false;
}