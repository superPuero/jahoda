#ifndef jahoda_gpu_surface
#define jahoda_gpu_surface

#include "gpu_physical_device.h"
#include "window.h"

da_declare(VkSurfaceFormatKHR, gpu_surface_format_da);
da_declare(VkPresentModeKHR, gpu_present_mode_da);

typedef struct
{
	VkSurfaceKHR handle;
	VkSurfaceFormatKHR surface_format;
	VkSurfaceCapabilitiesKHR capabilities;	
} gpu_surface;

gpu_surface gpu_surface_make(arena *temp_arena, window *window, gpu_instance *instance, gpu_physical_device *pdevice);
gpu_surface gpu_surface_refresh_capabilities(gpu_surface *surface, gpu_physical_device *pdevice);
void gpu_surface_release(gpu_instance *instance, gpu_surface *surface);
b8 gpu_surface_supports_present_format(gpu_surface *surface, gpu_physical_device *pdevice, arena *temp_arena, VkPresentModeKHR present_mode);

#endif