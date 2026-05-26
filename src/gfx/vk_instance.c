#include "vk_instance.h"


VkBool32 VKAPI_PTR debug_callback
(
	VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	
		infol(vulkan_verbose, "(verbose) %i, %s, %s", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		infol(vulkan_info, "%i, %s, %s", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		warnl(vulkan_warn, "%i, %s, %s", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		errl(vulkan_err, "%i, %s, %s", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		break;

	default:
		break;
	}

	return VK_FALSE;
}


void validate_required_layers(arena* temp_arena, cstr_da* required_layers)
{  
	uint32_t property_count;
	vkEnumerateInstanceLayerProperties(&property_count, NULL);

	da_declare(VkLayerProperties, vk_layer_properties_da);

	vk_layer_properties_da available_properties = {0};
	da_reserve(temp_arena, &available_properties, property_count);

	vkEnumerateInstanceLayerProperties(&property_count, available_properties.data);
	available_properties.occupied = property_count;
	
	da_foreach(required_layers)
	{
		bool8 present = false;
		da_foreach(&available_properties)	
		{
			if (strcmp(*required_layers->it, available_properties.it->layerName) == 0)
			{
				present = true;
				break;
			}
		}

		dbg_verify(present, "required layer %s is not found", *required_layers->it);
	}
}


void validate_required_extensions(arena* temp_arena, cstr_da* required_extension)
{  
	uint32_t extension_count;
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);

	da_declare(VkExtensionProperties, vk_extension_properties_da);

	vk_extension_properties_da available_properties = {0};
	da_reserve(temp_arena, &available_properties, extension_count);

	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, available_properties.data);
	available_properties.occupied = extension_count;
	
	da_foreach(required_extension)
	{
		bool8 present = false;
		da_foreach(&available_properties)	
		{
			if (strcmp(*required_extension->it, available_properties.it->extensionName) == 0)
			{
				present = true;
				break;
			}
		}

		dbg_verify(present, "required extension %s is not found", *required_extension->it);
	}
}

vk_instance vk_instance_make(arena* temp_arena)
{
	vk_instance out = {0};
	cstr_da required_layers = {0};
	cstr_da required_extensions = {0};

	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName = "unnamed";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "jahoda";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_ci = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instance_ci.pApplicationInfo = &app_info;

	#ifdef jahoda_debug
		#define X(l) da_append(temp_arena, &required_layers, #l);
		vk_instance_debug_layer_list
		#undef X		
	#endif

	validate_required_layers(temp_arena, &required_layers);

	instance_ci.enabledLayerCount = required_layers.occupied;
	instance_ci.ppEnabledLayerNames = required_layers.data;			

	u32 glfw_extension_count = 0;
	const char** glfw_extensions;

	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	for (u32 i = 0; i < glfw_extension_count; i++)
	{
		da_append(temp_arena, &required_extensions, glfw_extensions[i]);
	}

	#ifdef jahoda_debug
		#define X(e) da_append(temp_arena, &required_extensions, e);
		vk_instance_debug_extension_list
		#undef X	
	#endif

	validate_required_extensions(temp_arena, &required_extensions);

	instance_ci.enabledExtensionCount = required_extensions.occupied;
	instance_ci.ppEnabledExtensionNames = required_extensions.data;

	VkDebugUtilsMessengerCreateInfoEXT debug_util_messenger_ci = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debug_util_messenger_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_util_messenger_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debug_util_messenger_ci.pfnUserCallback = debug_callback;

	#ifdef jahoda_debug
		instance_ci.pNext = &debug_util_messenger_ci;
	#endif

	vk_check(vkCreateInstance(&instance_ci, NULL, &out.handle));

	#ifdef jahoda_debug
		PFN_vkCreateDebugUtilsMessengerEXT func = NULL;
		(func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(out.handle, "vkCreateDebugUtilsMessengerEXT"));
		dbg_verify(func, "");
		func(out.handle, &debug_util_messenger_ci, NULL, &out.debug_messenger);
	#endif

	return out;
}


void vk_instance_release(vk_instance* instance)
{
	#ifdef jahoda_debug	
		PFN_vkDestroyDebugUtilsMessengerEXT func = NULL;
		(func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->handle, "vkDestroyDebugUtilsMessengerEXT"));
		dbg_verify(func, "");
		func(instance->handle, instance->debug_messenger, NULL);
	#endif
	
	vkDestroyInstance(instance->handle, NULL);
}