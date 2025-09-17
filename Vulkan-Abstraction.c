#include "Vulkan-Abstraction.h"

/********************
 * Main Vulkan base *
 ********************/

vka_vulkan_t vka_vulkan_initialise()
{
	vka_vulkan_t vulkan;
	memset(&vulkan, 0, sizeof(vulkan));

	// Config:
	strcpy(vulkan.config.application_name, "Application");
	strcpy(vulkan.config.engine_name, "Engine");
	strcpy(vulkan.config.window_name, "Window");

	vulkan.config.application_version	= VK_MAKE_VERSION(1, 0, 0);
	vulkan.config.engine_version		= VK_MAKE_VERSION(1, 0, 0);

	vulkan.config.minimum_screen_width	= 640;
	vulkan.config.minimum_screen_height	= 480;

	vulkan.config.enabled_features_11.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	vulkan.config.enabled_features_11.pNext = NULL;

	vulkan.config.enabled_features_12.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan.config.enabled_features_12.pNext = &(vulkan.config.enabled_features_11);

	vulkan.config.enabled_features_13.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan.config.enabled_features_13.pNext = &(vulkan.config.enabled_features_12);

	// Main data:
	vulkan.window			= NULL;
	vulkan.screen_width		= 1080;
	vulkan.screen_height		= 720;

	vulkan.instance			= VK_NULL_HANDLE;
	vulkan.surface			= VK_NULL_HANDLE;

	vulkan.physical_device		= VK_NULL_HANDLE;
	vulkan.device			= VK_NULL_HANDLE;
	vulkan.graphics_family_index	= 100;
	vulkan.graphics_queue		= VK_NULL_HANDLE;
	vulkan.present_family_index	= 100;
	vulkan.present_queue		= VK_NULL_HANDLE;

	strcpy(vulkan.error_message, "");
	#ifdef VKA_DEBUG
	vulkan.debug_messenger		= VK_NULL_HANDLE;
	#endif

	return vulkan;
}

int vka_vulkan_setup(vka_vulkan_t *vulkan)
{
	int error;

	error = vka_create_window(vulkan);
	if (error) { return error; }

	error = volkInitialize();
	if (error) { return error; }

	#ifdef VKA_DEBUG
	error = vka_check_instance_layer_extension_support(vulkan);
	if (error) { return error; }
	#endif

	error = vka_create_instance(vulkan);
	if (error) { return error; }

	error = vka_create_surface(vulkan);
	if (error) { return error; }

	#ifdef VKA_DEBUG
	error = vka_create_debug_messenger(vulkan);
	if (error) { return error; }
	#endif

	error = vka_create_device(vulkan);
	if (error) { return error; }

	return VK_SUCCESS;
}

void vka_vulkan_shutdown(vka_vulkan_t *vulkan)
{
	vka_device_wait_idle(vulkan);

	if (vulkan->device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(vulkan->device, NULL);
		vulkan->device = VK_NULL_HANDLE;
	}

	#ifdef VKA_DEBUG
	if (vulkan->debug_messenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(vulkan->instance, vulkan->debug_messenger, NULL);
		vulkan->debug_messenger = VK_NULL_HANDLE;
	}
	#endif

	if (vulkan->surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vulkan->instance, vulkan->surface, NULL);
		vulkan->surface = VK_NULL_HANDLE;
	}

	if (vulkan->instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(vulkan->instance, NULL);
		vulkan->instance = VK_NULL_HANDLE;
	}

	if (vulkan->window != NULL) { SDL_DestroyWindow(vulkan->window); }
	vulkan->window = NULL;
	SDL_Quit();
}

void vka_print_vulkan(vka_vulkan_t *vulkan, FILE *file)
{

}

/****************************
 * Main Vulkan base helpers *
 ****************************/

int vka_create_window(vka_vulkan_t *vulkan)
{
	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not initialise SDL -> %s.\n", SDL_GetError());
		return VKA_ERROR;
	}

	vulkan->window = SDL_CreateWindow(vulkan->config.window_name, vulkan->screen_width,
			vulkan->screen_height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (vulkan->window == NULL)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not create window -> %s.\n", SDL_GetError());
		return VKA_ERROR;
	}

	if (SDL_SetWindowFullscreenMode(vulkan->window, NULL) != true)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not set window fullscreen mode -> %s.\n", SDL_GetError());
		return VKA_ERROR;
	}

	if (SDL_SetWindowMinimumSize(vulkan->window, vulkan->config.minimum_screen_width,
					vulkan->config.minimum_screen_height) != true)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not set window minimum size -> %s.\n", SDL_GetError());
		return VKA_ERROR;
	}

	return VK_SUCCESS;
}

int vka_create_instance(vka_vulkan_t *vulkan)
{
	int error;

	VkApplicationInfo application_info;
	memset(&application_info, 0, sizeof(application_info));
	application_info.sType			= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pNext			= NULL;
	application_info.pApplicationName	= vulkan->config.application_name;
	application_info.applicationVersion	= vulkan->config.application_version;
	application_info.pEngineName		= vulkan->config.engine_name;
	application_info.engineVersion		= vulkan->config.engine_version;
	application_info.apiVersion		= VK_MAKE_API_VERSION(0, 1, 3, 0);

	uint32_t layer_count = 0;
	char *layers[1];
	uint32_t extension_count;
	char const * const *SDL_extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);

	uint32_t debug_extension_count = 0;
	#ifdef VKA_DEBUG
	debug_extension_count = 1;
	layer_count = 1;
	layers[0] = "VK_LAYER_KHRONOS_validation";
	#endif
	extension_count += debug_extension_count;

	char **extensions = malloc(extension_count * sizeof(char *));
	if (extensions == NULL)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not allocate memory for extension names.\n");
		return VKA_ERROR;
	}
	for (uint32_t i = 0; i < extension_count; i++)
	{
		extensions[i] = malloc(VKA_MAX_NAME_LENGTH * sizeof(char));
		if (extensions[i] == NULL)
		{
			for (uint32_t j = 0; j < i; j++) { free(extensions[j]); }
			free(extensions);
			snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
				"Could not allocate memory for extension names.\n");
			return VKA_ERROR;
		}
	}

	#ifdef VKA_DEBUG
	strcpy(extensions[0], VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	#endif
	for (uint32_t i = debug_extension_count; i < extension_count; i++)
	{
		strcpy(extensions[i], SDL_extensions[i - debug_extension_count]);
	}

	VkInstanceCreateInfo instance_create_info;
	memset(&instance_create_info, 0, sizeof(instance_create_info));
	instance_create_info.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext			= NULL;
	instance_create_info.flags			= 0;
	instance_create_info.pApplicationInfo		= &application_info;
	instance_create_info.enabledLayerCount		= layer_count;
	instance_create_info.ppEnabledLayerNames	= (const char **)layers;
	instance_create_info.enabledExtensionCount	= extension_count;
	instance_create_info.ppEnabledExtensionNames	= (const char **)extensions;

	error = vkCreateInstance(&instance_create_info, NULL, &(vulkan->instance));
	if (error || (vulkan->instance == VK_NULL_HANDLE))
	{
		for (uint32_t i = 0; i < extension_count; i++) { free(extensions[i]); }
		free(extensions);
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not create Vulkan instance.\n");
		vulkan->instance = VK_NULL_HANDLE;
		return error;
	}

	for (uint32_t i = 0; i < extension_count; i++) { free(extensions[i]); }
	free(extensions);

	volkLoadInstanceOnly(vulkan->instance);

	return VK_SUCCESS;
}

int vka_create_surface(vka_vulkan_t *vulkan)
{
	return VK_SUCCESS;
}

int vka_create_device(vka_vulkan_t *vulkan)
{
	return VK_SUCCESS;
}

/***********
 * Utility *
 ***********/

void vka_device_wait_idle(vka_vulkan_t *vulkan)
{
	if (vulkan->device == VK_NULL_HANDLE) { return; }
	vkDeviceWaitIdle(vulkan->device);
}

/*********
 * Debug *
 *********/

#ifdef VKA_DEBUG
int vka_check_instance_layer_extension_support(vka_vulkan_t *vulkan)
{
	int error;

	// Check layers:
	uint32_t num_layers;
	error = vkEnumerateInstanceLayerProperties(&num_layers, NULL);
	if (error)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not enumerate instance layers.\n");
		return error;
	}

	VkLayerProperties *layers = malloc(num_layers * sizeof(VkLayerProperties));
	if (layers == NULL)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not allocate memory for instance layer properties.\n");
		return VKA_ERROR;
	}
	error = vkEnumerateInstanceLayerProperties(&num_layers, layers);
	if (error)
	{
		free(layers);
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
				"Could not get instance layers.\n");
		return error;
	}

	int layer_supported = 0;
	for (uint32_t i = 0; i < num_layers; i++)
	{
		if (strcmp("VK_LAYER_KHRONOS_validation", layers[i].layerName) == 0)
		{
			layer_supported = 1;
			break;
		}
	}
	if (layer_supported == 0)
	{
		free(layers);
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Required layer \"VK_LAYER_KHRONOS_validation\" not supported.\n");
		return VKA_ERROR;
	}

	free(layers);

	// Check extensions:
	uint32_t num_extensions;
	error = vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL);
	if (error)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not enumerate instance extensions.\n");
		return error;
	}

	VkExtensionProperties *extensions = malloc(num_extensions * sizeof(VkExtensionProperties));
	if (extensions == NULL)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Could not allocate memory for instance extension properties.\n");
		return VKA_ERROR;
	}
	error = vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions);
	if (error)
	{
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
				"Could not get instance extensions.\n");
		return error;
	}

	int extension_supported = 0;
	for (uint32_t i = 0; i < num_extensions; i++)
	{
		if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensions[i].extensionName) == 0)
		{
			extension_supported = 1;
			break;
		}
	}
	if (extension_supported == 0)
	{
		free(extensions);
		snprintf(vulkan->error_message, VKA_ERROR_MESSAGE_LENGTH,
			"Required extension \"%s\" not supported.\n",
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		return VKA_ERROR;
	}

	free(extensions);

	return VK_SUCCESS;
}

int vka_create_debug_messenger(vka_vulkan_t *vulkan)
{
	return VK_SUCCESS;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	VkDebugUtilsMessengerCallbackDataEXT const *data,
	void *user_pointer)
{
	char *severity_string = vka_debug_get_severity(severity);
	char *type_string = vka_debug_get_type(type);

	fprintf(stderr, "%s (%s): %s (%d)\n%s\n--\n", severity_string, type_string,
		data->pMessageIdName, data->messageIdNumber, data->pMessage);

	return VK_FALSE;
}

char *vka_debug_get_severity(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{ return "SEVERITY_VERBOSE"; }
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{ return "SEVERITY_INFO"; }
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{ return "SEVERITY_WARNING"; }
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{ return "SEVERITY_ERROR"; }
	else
	{ return "UNKNOWN SEVERITY TYPE"; }
}

char *vka_debug_get_type(VkDebugUtilsMessageTypeFlagsEXT type)
{
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{ return "GENERAL"; }
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{ return "VALIDATION"; }
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{ return "PERFORMANCE"; }
	else
	{ return "UNKNOWN MESSAGE TYPE"; }
}
#endif
