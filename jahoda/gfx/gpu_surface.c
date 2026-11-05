#include "gpu_surface.h"

gpu_surface gpu_surface_make(arena *temp_arena, window *window, gpu_instance *instance, gpu_physical_device *pdevice)
{
	gpu_surface out = {0};

	gpu_check(glfwCreateWindowSurface(instance->handle, window->handle, NULL, &out.handle));

	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR func = NULL;
	(func = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance->handle, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
	dbg_verify(func, "");
	gpu_check(func(pdevice->handle, out.handle, &out.capabilities));

	u32 format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice->handle, out.handle, &format_count, NULL);

	gpu_surface_format_da avalible_surafce_formats = {0};
	da_resize(&avalible_surafce_formats, temp_arena, format_count);

	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice->handle, out.handle, &format_count, avalible_surafce_formats.data);

	out.surface_format = avalible_surafce_formats.data[0];

	for da_each(it, &avalible_surafce_formats)
	{
		VkSurfaceFormatKHR format = *it;
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			out.surface_format = format;
			break;
		}
	}

	return out;
}

b8 gpu_surface_supports_present_format(gpu_surface *surface, gpu_physical_device *pdevice, arena *temp_arena, VkPresentModeKHR present_mode)
{
	u32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice->handle, surface->handle, &present_mode_count, NULL);
	gpu_present_mode_da avalible_present_modes = {0};
	da_resize(&avalible_present_modes, temp_arena, present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice->handle, surface->handle, &present_mode_count, avalible_present_modes.data);

	b8 found = false;
	for da_each(it, &avalible_present_modes)
	{
		VkPresentModeKHR mode = *it;

		if (mode == present_mode)
		{
			return true;
		}
	}

	return false;
}

gpu_surface gpu_surface_refresh_capabilities(gpu_surface *surface, gpu_physical_device *pdevice)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice->handle, surface->handle, &surface->capabilities);
}


void gpu_surface_release(gpu_instance *instance, gpu_surface *surface)
{
	vkDestroySurfaceKHR(instance->handle, surface->handle, NULL);
}