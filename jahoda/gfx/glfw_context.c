#include "glfw_context.h"
#include "gpu_glfw.h"

glfw_context glfw_context_make()
{
	glfwInit();   
    
    verifyl(glfw, glfwInit(), "initialization failed");
    verifyl(glfw, glfwVulkanSupported(), "vulkan is not supported");

	return (glfw_context){};
}

void glfw_context_release(glfw_context *ctx)
{
	glfwTerminate();
}