#include "vk_surface.h"

vk_surface vk_surface_make(arena *temp_arena, window *window, vk_instance *instance, vk_physical_device *pdevice)
{
	vk_surface out = {0};

	vk_check(glfwCreateWindowSurface(instance->handle, window->handle, NULL, &out.handle));

	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR func = NULL;
	(func = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(instance->handle, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
	dbg_verify(func, "");
	vk_check(func(pdevice->handle, out.handle, &out.capabilities));

	u32 format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice->handle, out.handle, &format_count, NULL);

	vk_surface_format_da avalible_surafce_formats = {0};
	da_resize(temp_arena, &avalible_surafce_formats, format_count);

	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice->handle, out.handle, &format_count, avalible_surafce_formats.data);

	out.surface_format = avalible_surafce_formats.data[0];

	for da_each(&avalible_surafce_formats, it)
	{
		VkSurfaceFormatKHR format = *it;
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			out.surface_format = format;
			break;
		}
	}

	u32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice->handle, out.handle, &present_mode_count, NULL);
	vk_present_mode_da avalible_present_modes = {0};
	da_resize(temp_arena, &avalible_present_modes, present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice->handle, out.handle, &present_mode_count, avalible_present_modes.data);

	for da_each(&avalible_present_modes, it)
	{
		VkPresentModeKHR mode = *it;

		if (mode == jahoda_vk_present_mode)
		{
			out.present_mode = mode;
			break;
		}
	}
	bool8 found = (out.present_mode == jahoda_vk_present_mode);
	dbg_verify(found, "requested present mode %s is not supported", jahoda_vk_present_mode_str);

	return out;
}
vk_surface vk_surface_refresh_capabilities(vk_surface *surface, vk_physical_device *pdevice)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice->handle, surface->handle, &surface->capabilities);
}


void vk_surface_release(vk_instance *instance, vk_surface *surface)
{
	vkDestroySurfaceKHR(instance->handle, surface->handle, NULL);
}