#ifndef jahoda_vk_surface
#define jahoda_vk_surface

#include "vk_physical_device.h"
#include "window.h"

#define jahoda_vk_present_mode VK_PRESENT_MODE_FIFO_KHR
// #define jahoda_vk_present_mode VK_PRESENT_MODE_MAILBOX_KHR
// #define jahoda_vk_present_mode VK_PRESENT_MODE_IMMEDIATE_KHR
 
#define jahoda_vk_present_mode_str xstringify(jahoda_vk_present_mode)

da_declare(VkSurfaceFormatKHR, vk_surface_format_da);
da_declare(VkPresentModeKHR, vk_present_mode_da);

typedef struct
{
	VkSurfaceKHR handle;
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;
	VkSurfaceCapabilitiesKHR capabilities;	
} vk_surface;

vk_surface vk_surface_make(arena* temp_arena, window* window, vk_instance* instance, vk_physical_device* pdevice);
vk_surface vk_surface_refresh_capabilities(vk_surface* surface, vk_physical_device* pdevice);
void vk_surface_release(vk_instance* instance, vk_surface* surface);


#endif