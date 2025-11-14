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
	vulkan.config.enabled_features_12.pNext = NULL;

	vulkan.config.enabled_features_13.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan.config.enabled_features_13.pNext = NULL;

	vulkan.config.max_memory_allocation_size = 1073741824;
	vulkan.config.max_sampler_descriptors = 1024;

	// Main data:
	vulkan.window			= NULL;
	vulkan.instance			= VK_NULL_HANDLE;
	vulkan.surface			= VK_NULL_HANDLE;

	vulkan.physical_device		= VK_NULL_HANDLE;
	vulkan.device			= VK_NULL_HANDLE;
	vulkan.graphics_family_index	= 100;
	vulkan.graphics_queue		= VK_NULL_HANDLE;
	vulkan.present_family_index	= 100;
	vulkan.present_queue		= VK_NULL_HANDLE;

	vulkan.command_pool		= VK_NULL_HANDLE;

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vulkan.command_buffers[i]	= VK_NULL_HANDLE;
		vulkan.command_fences[i]	= VK_NULL_HANDLE;

		vulkan.image_available[i]	= VK_NULL_HANDLE;
		vulkan.render_complete[i]	= VK_NULL_HANDLE;
	}

	vulkan.num_swapchain_images	= 0;
	vulkan.swapchain_format		= VK_FORMAT_UNDEFINED;
	vulkan.swapchain_extent.width	= 0;
	vulkan.swapchain_extent.height	= 0;
	vulkan.swapchain		= VK_NULL_HANDLE;
	vulkan.swapchain_images		= NULL;
	vulkan.swapchain_image_views	= NULL;

	vulkan.recreate_swapchain	= 0;
	vulkan.recreate_pipelines	= 0;
	vulkan.current_frame		= 0;
	vulkan.current_swapchain_index	= 0;

	strcpy(vulkan.error_message, "");
	#ifdef VKA_DEBUG
	vulkan.debug_messenger		= VK_NULL_HANDLE;
	#endif

	return vulkan;
}

int vka_vulkan_setup(vka_vulkan_t *vulkan)
{
	vulkan->config.enabled_features_12.pNext = &(vulkan->config.enabled_features_11);
	vulkan->config.enabled_features_13.pNext = &(vulkan->config.enabled_features_12);

	if (vka_create_window(vulkan)) { return -1; }
	if (volkInitialize()) { return -1; }

	#ifdef VKA_DEBUG
	if (vka_check_instance_layer_extension_support(vulkan)) { return -1; }
	#endif

	if (vka_create_instance(vulkan)) { return -1; }
	if (vka_create_surface(vulkan)) { return -1; }

	#ifdef VKA_DEBUG
	if (vka_create_debug_messenger(vulkan)) { return -1; }
	#endif

	if (vka_create_device(vulkan)) { return -1; }
	if (vka_create_command_pool(vulkan)) { return -1; }
	if (vka_create_command_buffers(vulkan)) { return -1; }
	if (vka_create_command_fences(vulkan)) { return -1; }
	if (vka_create_semaphores(vulkan)) { return -1; }
	if (vka_create_swapchain(vulkan)) { return -1; }
	vulkan->recreate_pipelines = 0;

	return 0;
}

void vka_vulkan_shutdown(vka_vulkan_t *vulkan)
{
	vka_device_wait_idle(vulkan);

	if (vulkan->swapchain_image_views)
	{
		for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
		{
			if (vulkan->swapchain_image_views[i] != VK_NULL_HANDLE)
			{
				vkDestroyImageView(vulkan->device,
					vulkan->swapchain_image_views[i], NULL);
				vulkan->swapchain_image_views[i] = VK_NULL_HANDLE;
			}
		}
		free(vulkan->swapchain_image_views);
		vulkan->swapchain_image_views = NULL;
	}

	if (vulkan->swapchain_images)
	{
		free(vulkan->swapchain_images);
		vulkan->swapchain_images = NULL;
	}

	if (vulkan->swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vulkan->device, vulkan->swapchain, NULL);
		vulkan->swapchain = VK_NULL_HANDLE;
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vulkan->image_available[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(vulkan->device, vulkan->image_available[i], NULL);
			vulkan->image_available[i] = VK_NULL_HANDLE;
		}
		if (vulkan->render_complete[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(vulkan->device, vulkan->render_complete[i], NULL);
			vulkan->render_complete[i] = VK_NULL_HANDLE;
		}
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vulkan->command_fences[i] != VK_NULL_HANDLE)
		{
			vkDestroyFence(vulkan->device, vulkan->command_fences[i], NULL);
			vulkan->command_fences[i] = VK_NULL_HANDLE;
		}
	}

	if (vulkan->command_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(vulkan->device, vulkan->command_pool, NULL);
		vulkan->command_pool = VK_NULL_HANDLE;
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vulkan->command_buffers[i] = VK_NULL_HANDLE;
	}

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

	if (vulkan->window) { SDL_DestroyWindow(vulkan->window); }
	vulkan->window = NULL;
	SDL_Quit();
}

/****************************
 * Main Vulkan base helpers *
 ****************************/

int vka_create_window(vka_vulkan_t *vulkan)
{
	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not initialise SDL -> %s.", SDL_GetError());
		return -1;
	}

	vulkan->window = SDL_CreateWindow(vulkan->config.window_name,
		vulkan->config.minimum_screen_width, vulkan->config.minimum_screen_height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!vulkan->window)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not create window -> %s.", SDL_GetError());
		return -1;
	}

	if (SDL_SetWindowFullscreenMode(vulkan->window, NULL) != true)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not set window fullscreen mode -> %s.", SDL_GetError());
		return -1;
	}

	if (SDL_SetWindowMinimumSize(vulkan->window, vulkan->config.minimum_screen_width,
					vulkan->config.minimum_screen_height) != true)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not set window minimum size -> %s.", SDL_GetError());
		return -1;
	}

	return 0;
}

int vka_create_instance(vka_vulkan_t *vulkan)
{
	VkApplicationInfo application_info;
	memset(&application_info, 0, sizeof(application_info));
	application_info.sType			= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pNext			= NULL;
	application_info.pApplicationName	= vulkan->config.application_name;
	application_info.applicationVersion	= vulkan->config.application_version;
	application_info.pEngineName		= vulkan->config.engine_name;
	application_info.engineVersion		= vulkan->config.engine_version;
	application_info.apiVersion		= VK_MAKE_API_VERSION(0, VKA_API_VERSION_MAJOR,
								VKA_API_VERSION_MINOR, 0);

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
	if (!extensions)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for extension names.");
		return -1;
	}
	for (uint32_t i = 0; i < extension_count; i++)
	{
		extensions[i] = malloc(MAX_NAME_LENGTH * sizeof(char));
		if (!extensions[i])
		{
			for (uint32_t j = 0; j < i; j++) { free(extensions[j]); }
			free(extensions);
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
				"Could not allocate memory for extension names.");
			return -1;
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

	if (vkCreateInstance(&instance_create_info, NULL, &(vulkan->instance)) != VK_SUCCESS)
	{
		for (uint32_t i = 0; i < extension_count; i++) { free(extensions[i]); }
		free(extensions);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not create Vulkan instance.");
		vulkan->instance = VK_NULL_HANDLE;
		return -1;
	}

	for (uint32_t i = 0; i < extension_count; i++) { free(extensions[i]); }
	free(extensions);

	volkLoadInstanceOnly(vulkan->instance);

	return 0;
}

int vka_create_surface(vka_vulkan_t *vulkan)
{
	if (SDL_Vulkan_CreateSurface(vulkan->window, vulkan->instance,
				NULL, &(vulkan->surface)) != true)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not create surface -> %s.", SDL_GetError());
		return -1;
	}

	return 0;
}

int vka_create_device(vka_vulkan_t *vulkan)
{
	uint32_t num_physical_devices;
	if (vkEnumeratePhysicalDevices(vulkan->instance, &num_physical_devices, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not enumerate physical devices.");
		return -1;
	}

	VkPhysicalDevice *physical_devices = malloc(num_physical_devices *
						sizeof(VkPhysicalDevice));
	if (!physical_devices)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for physical device array.");
		return -1;
	}

	if (vkEnumeratePhysicalDevices(vulkan->instance, &num_physical_devices,
					physical_devices) != VK_SUCCESS)
	{
		free(physical_devices);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get array of physical devices.");
		return -1;
	}

	int best_score = -1;
	uint32_t graphics_family_index;
	uint32_t present_family_index;
	VkDeviceSize max_memory_allocation_size;
	VkBool32 sampler_anisotropy;

	for (uint32_t i = 0; i < num_physical_devices; i++)
	{
		int score = vka_score_physical_device(vulkan, physical_devices[i],
			&graphics_family_index, &present_family_index,
			&max_memory_allocation_size, &sampler_anisotropy);
		if (score > best_score)
		{
			best_score = score;
			vulkan->physical_device = physical_devices[i];
			vulkan->graphics_family_index = graphics_family_index;
			vulkan->present_family_index = present_family_index;
			vulkan->config.max_memory_allocation_size = max_memory_allocation_size;
			vulkan->config.enabled_features.samplerAnisotropy = sampler_anisotropy;
		}
	}

	free(physical_devices);

	if (best_score < 0)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"No suitable physical device found.");
		return -1;
	}

	uint32_t num_queue_families;
	if (vulkan->graphics_family_index == vulkan->present_family_index)
	{
		num_queue_families = 1;
	}
	else { num_queue_families = 2; }

	VkDeviceQueueCreateInfo *queue_info = malloc(num_queue_families *
					sizeof(VkDeviceQueueCreateInfo));
	if (!queue_info)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for device queue info.");
		return -1;
	}
	memset(queue_info, 0, num_queue_families * sizeof(VkDeviceQueueCreateInfo));
	float queue_priorities[1] = { 1.f };

	for (uint32_t i = 0; i < num_queue_families; i++)
	{
		queue_info[i].sType		= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[i].pNext		= NULL;
		queue_info[i].flags		= 0;
		queue_info[i].pQueuePriorities	= queue_priorities;

		if (i == 0)
		{
			queue_info[i].queueFamilyIndex	= vulkan->graphics_family_index;
			queue_info[i].queueCount	= 1;
		}
		else
		{
			queue_info[i].queueFamilyIndex	= vulkan->present_family_index;
			queue_info[i].queueCount	= 1;
		}
	}

	VkPhysicalDeviceFeatures2 enabled_features;
	memset(&enabled_features, 0, sizeof(enabled_features));
	enabled_features.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	enabled_features.pNext		= &(vulkan->config.enabled_features_13);
	enabled_features.features	= vulkan->config.enabled_features;

	char *enabled_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo device_info;
	memset(&device_info, 0, sizeof(device_info));
	device_info.sType			= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext			= &enabled_features;
	device_info.flags			= 0;
	device_info.queueCreateInfoCount	= num_queue_families;
	device_info.pQueueCreateInfos		= queue_info;
	device_info.enabledLayerCount		= 0;
	device_info.ppEnabledLayerNames		= NULL;
	device_info.enabledExtensionCount	= 1;
	device_info.ppEnabledExtensionNames	= (const char **)enabled_extensions;
	device_info.pEnabledFeatures		= NULL;

	if (vkCreateDevice(vulkan->physical_device, &device_info, NULL,
				&(vulkan->device)) != VK_SUCCESS)
	{
		free(queue_info);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH, "Could not create device.");
		return -1;
	}

	free(queue_info);

	volkLoadDevice(vulkan->device);

	vkGetDeviceQueue(vulkan->device, vulkan->graphics_family_index, 0,
					&(vulkan->graphics_queue));
	if (vulkan->graphics_queue == VK_NULL_HANDLE)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not retrieve graphics queue.");
		return -1;
	}

	vkGetDeviceQueue(vulkan->device, vulkan->present_family_index, 0,
					&(vulkan->present_queue));
	if (vulkan->present_queue == VK_NULL_HANDLE)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not retrieve present queue.");
		return -1;
	}

	return 0;
}

int vka_score_physical_device(vka_vulkan_t *vulkan, VkPhysicalDevice physical_device,
	uint32_t *graphics_family_index, uint32_t *present_family_index,
	VkDeviceSize *max_memory_allocation_size, VkBool32 *sampler_anisotropy)
{
	int score = 0;

	VkPhysicalDeviceMaintenance3Properties maintenance_properties;
	memset(&maintenance_properties, 0, sizeof(maintenance_properties));
	maintenance_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
	maintenance_properties.pNext = NULL;

	VkPhysicalDeviceProperties2 get_properties;
	memset(&get_properties, 0, sizeof(get_properties));
	get_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	get_properties.pNext = &maintenance_properties;

	vkGetPhysicalDeviceProperties2(physical_device, &get_properties);

	*max_memory_allocation_size = maintenance_properties.maxMemoryAllocationSize;

	uint32_t version_major = VK_API_VERSION_MAJOR(get_properties.properties.apiVersion);
	uint32_t version_minor = VK_API_VERSION_MINOR(get_properties.properties.apiVersion);

	if ((version_major < VKA_API_VERSION_MAJOR) ||
		((version_major == VKA_API_VERSION_MAJOR) &&
		(version_minor < VKA_API_VERSION_MINOR)))
	{
		return -1;
	}

	if (get_properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 10;
	}

	uint32_t num_supported_extensions;
	if (vkEnumerateDeviceExtensionProperties(physical_device, NULL,
			&num_supported_extensions, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not enumerate extensions for physical device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	VkExtensionProperties *supported_extensions = malloc(num_supported_extensions *
							sizeof(VkExtensionProperties));
	if (!supported_extensions)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for extensions for physical device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	if (vkEnumerateDeviceExtensionProperties(physical_device, NULL,
		&num_supported_extensions, supported_extensions) != VK_SUCCESS)
	{
		free(supported_extensions);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get array of supported extensions for physical device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	int supported = 0;
	for (uint32_t i = 0; i < num_supported_extensions; i++)
	{
		if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			supported_extensions[i].extensionName) == 0)
		{
			supported = 1;
			break;
		}
	}
	if (!supported)
	{
		free(supported_extensions);
		return -1;
	}

	free(supported_extensions);

	VkPhysicalDeviceVulkan11Features supported_features_11;
	memset(&supported_features_11, 0, sizeof(supported_features_11));
	supported_features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	supported_features_11.pNext = NULL;

	VkPhysicalDeviceVulkan12Features supported_features_12;
	memset(&supported_features_12, 0, sizeof(supported_features_12));
	supported_features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	supported_features_12.pNext = &supported_features_11;

	VkPhysicalDeviceVulkan13Features supported_features_13;
	memset(&supported_features_13, 0, sizeof(supported_features_13));
	supported_features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	supported_features_13.pNext = &supported_features_12;

	VkPhysicalDeviceFeatures2 supported_features;
	memset(&supported_features, 0, sizeof(supported_features));
	supported_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	supported_features.pNext = &supported_features_13;

	vkGetPhysicalDeviceFeatures2(physical_device, &supported_features);

	// Deal with sampler anisotropy separately as it's not mandatory, but desirable:
	*sampler_anisotropy = supported_features.features.samplerAnisotropy;
	if (*sampler_anisotropy == VK_TRUE) { score += 2; }

	// Compare desired features with supported features, bitwise (exclude anisotropy):
	VkPhysicalDeviceFeatures desired_features = vulkan->config.enabled_features;
	desired_features.samplerAnisotropy = VK_FALSE;

	VkPhysicalDeviceVulkan11Features desired_features_11 = vulkan->config.enabled_features_11;
	desired_features_11.pNext = NULL;
	supported_features_11.pNext = NULL;

	VkPhysicalDeviceVulkan12Features desired_features_12 = vulkan->config.enabled_features_12;
	desired_features_12.pNext = NULL;
	supported_features_12.pNext = NULL;

	VkPhysicalDeviceVulkan13Features desired_features_13 = vulkan->config.enabled_features_13;
	desired_features_13.pNext = NULL;
	supported_features_13.pNext = NULL;

	size_t sizes_features[4] = { sizeof(VkPhysicalDeviceFeatures),
					sizeof(VkPhysicalDeviceVulkan11Features),
					sizeof(VkPhysicalDeviceVulkan12Features),
					sizeof(VkPhysicalDeviceVulkan13Features) };
	size_t total_size = sizes_features[0] + sizes_features[1] +
			sizes_features[2] + sizes_features[3];
	void *features[8] = { (void *)(&desired_features),
				(void *)(&desired_features_11),
				(void *)(&desired_features_12),
				(void *)(&desired_features_13),
				(void *)(&(supported_features.features)),
				(void *)(&supported_features_11),
				(void *)(&supported_features_12),
				(void *)(&supported_features_13) };

	uint8_t *desired_features_array = malloc(total_size);
	if (!desired_features_array)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for feature comparison for device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	uint8_t *supported_features_array = malloc(total_size);
	if (!supported_features_array)
	{
		free(desired_features_array);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for feature comparison for device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	size_t offset = 0;
	for (int i = 0; i < 4; i++)
	{
		memcpy(desired_features_array + offset, features[i], sizes_features[i]);
		memcpy(supported_features_array + offset, features[i + 4], sizes_features[i]);
		offset += sizes_features[i];
	}

	for (size_t i = 0; i < total_size; i++)
	{
		if ((desired_features_array[i] & supported_features_array[i]) !=
						desired_features_array[i])
		{
			free(desired_features_array);
			free(supported_features_array);
			return -1;
		}
	}

	free(desired_features_array);
	free(supported_features_array);

	// TODO - check maxPerSetDescriptors and score if >= config. Outweigh anisotropy.

	int graphics_queue_family_found = 0;
	int present_queue_family_found = 0;
	VkQueueFlags graphics_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

	uint32_t num_queue_families;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);
	VkQueueFamilyProperties *queue_families = malloc(num_queue_families *
					sizeof(VkQueueFamilyProperties));
	if (!queue_families)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for queue families for device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families,
								queue_families);

	for (uint32_t i = 0; i < num_queue_families; i++)
	{
		int queues_found = 0;

		if ((graphics_flags & queue_families[i].queueFlags) == graphics_flags)
		{
			queues_found++;
			if (!graphics_queue_family_found)
			{
				*graphics_family_index = i;
				graphics_queue_family_found = 1;
			}
		}

		VkBool32 present_supported = VK_FALSE;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i,
			vulkan->surface, &present_supported) != VK_SUCCESS)
		{
			continue;
		}
		if (present_supported == VK_TRUE)
		{
			queues_found++;
			if (!present_queue_family_found)
			{
				*present_family_index = i;
				present_queue_family_found = 1;
			}
		}

		if (queues_found == 2)
		{
			*graphics_family_index = i;
			*present_family_index = i;
			break;
		}
	}
	if (!graphics_queue_family_found || !present_queue_family_found)
	{
		free(queue_families);
		return -1;
	}

	free(queue_families);

	return score;
}

int vka_create_command_pool(vka_vulkan_t *vulkan)
{
	// TODO - is VK_COMMAND_POOL_CREATE_TRANSIENT_BIT needed?
	VkCommandPoolCreateInfo pool_info;
	memset(&pool_info, 0, sizeof(pool_info));
	pool_info.sType			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.pNext			= NULL;
	pool_info.flags			= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
						VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex	= vulkan->graphics_family_index;

	if (vkCreateCommandPool(vulkan->device, &pool_info, NULL,
			&(vulkan->command_pool)) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH, "Could not create command pool.");
		return -1;
	}

	return 0;
}

int vka_create_command_buffers(vka_vulkan_t *vulkan)
{
	for (uint32_t i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo alloc_info;
		memset(&alloc_info, 0, sizeof(alloc_info));
		alloc_info.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.pNext		= NULL;
		alloc_info.commandPool		= vulkan->command_pool;
		alloc_info.level		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount	= 1;

		if (vkAllocateCommandBuffers(vulkan->device, &alloc_info,
			&(vulkan->command_buffers[i])) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
				"Could not allocate command buffer.");
			return -1;
		}
	}

	return 0;
}

int vka_create_command_fences(vka_vulkan_t *vulkan)
{
	for (uint32_t i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkFenceCreateInfo fence_info;
		memset(&fence_info, 0, sizeof(fence_info));
		fence_info.sType	= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.pNext	= NULL;
		fence_info.flags	= VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateFence(vulkan->device, &fence_info, NULL,
			&(vulkan->command_fences[i])) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
					"Could not create fence.");
			return -1;
		}
	}

	return 0;
}

int vka_create_semaphores(vka_vulkan_t *vulkan)
{
	VkSemaphoreCreateInfo semaphore_info;
	memset(&semaphore_info, 0, sizeof(semaphore_info));
	semaphore_info.sType	= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.pNext	= NULL;
	semaphore_info.flags	= 0;

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(vulkan->device, &semaphore_info, NULL,
			&(vulkan->image_available[i])) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
				"Could not create semaphore: \"Image available\".");
			return -1;
		}

		if (vkCreateSemaphore(vulkan->device, &semaphore_info, NULL,
			&(vulkan->render_complete[i])) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
				"Could not create semaphore: \"Render complete\".");
			return -1;
		}
	}

	return 0;
}

int vka_create_swapchain(vka_vulkan_t *vulkan)
{
	// Used for initial creation AND recreation of swapchain.
	vka_device_wait_idle(vulkan);

	if (vulkan->swapchain_image_views)
	{
		for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
		{
			if (vulkan->swapchain_image_views[i] != VK_NULL_HANDLE)
			{
				vkDestroyImageView(vulkan->device,
					vulkan->swapchain_image_views[i], NULL);
				vulkan->swapchain_image_views[i] = VK_NULL_HANDLE;
			}
		}
		free(vulkan->swapchain_image_views);
		vulkan->swapchain_image_views = NULL;
	}

	if (vulkan->swapchain_images)
	{
		free(vulkan->swapchain_images);
		vulkan->swapchain_images = NULL;
	}

	VkFormat old_format = vulkan->swapchain_format;
	VkSwapchainKHR old_swapchain = vulkan->swapchain;

	uint32_t num_formats;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physical_device, vulkan->surface,
							&num_formats, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not enumerate surface formats.");
		return -1;
	}

	VkSurfaceFormatKHR *formats = malloc(num_formats * sizeof(VkSurfaceFormatKHR));
	if (!formats)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for array of surface formats.");
		return -1;
	}

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physical_device, vulkan->surface,
						&num_formats, formats) != VK_SUCCESS)
	{
		free(formats);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get array of surface formats.");
		return -1;
	}

	if (num_formats < 1)
	{
		free(formats);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"No surface formats to choose from.");
		return -1;
	}

	VkSurfaceFormatKHR chosen_format = formats[0];
	for (uint32_t i = 0; i < num_formats; i++)
	{
		if ((formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) &&
			(formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
		{
			chosen_format = formats[i];
			break;
		}

		if ((formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) &&
			(formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
		{
			chosen_format = formats[i];
		}
	}

	vulkan->swapchain_format = chosen_format.format;
	free(formats);

	uint32_t num_modes;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physical_device, vulkan->surface,
							&num_modes, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not enumerate surface present modes.");
		return -1;
	}

	VkPresentModeKHR *modes = malloc(num_modes * sizeof(VkPresentModeKHR));
	if (!modes)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for array of surface present modes.");
		return -1;
	}

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physical_device, vulkan->surface,
							&num_modes, modes) != VK_SUCCESS)
	{
		free(modes);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get array of surface present modes.");
		return -1;
	}

	if (num_modes < 1)
	{
		free(modes);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"No surface present modes to choose from.");
		return -1;
	}

	VkPresentModeKHR chosen_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < num_modes; i++)
	{
		if (modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
		{
			chosen_mode = modes[i];
			break;
		}
	}

	free(modes);

	VkSurfaceCapabilitiesKHR capabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physical_device, vulkan->surface,
								&capabilities) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get surface capabilities for the device.");
		return -1;
	}

	uint32_t num_images = 2;
	if (num_images < (capabilities.minImageCount + 1))
	{
		num_images = capabilities.minImageCount + 1;
	}
	if ((num_images > capabilities.maxImageCount) && (capabilities.maxImageCount > 0))
	{
		num_images = capabilities.maxImageCount;
	}

	vulkan->swapchain_extent = capabilities.currentExtent;
	if (vulkan->swapchain_extent.width == UINT32_MAX)
	{
		int width;
		int height;
		SDL_GetWindowSizeInPixels(vulkan->window, &width, &height);
		vulkan->swapchain_extent.width = width;
		vulkan->swapchain_extent.height = height;

		if (vulkan->swapchain_extent.width < capabilities.minImageExtent.width)
		{
			vulkan->swapchain_extent.width = capabilities.minImageExtent.width;
		}
		if (vulkan->swapchain_extent.width > capabilities.maxImageExtent.width)
		{
			vulkan->swapchain_extent.width = capabilities.maxImageExtent.width;
		}
		if (vulkan->swapchain_extent.height < capabilities.minImageExtent.height)
		{
			vulkan->swapchain_extent.height = capabilities.minImageExtent.height;
		}
		if (vulkan->swapchain_extent.height > capabilities.maxImageExtent.height)
		{
			vulkan->swapchain_extent.height = capabilities.maxImageExtent.height;
		}
	}

	VkSwapchainCreateInfoKHR swapchain_info;
	memset(&swapchain_info, 0, sizeof(swapchain_info));
	swapchain_info.sType		= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext		= NULL;
	swapchain_info.flags		= 0;
	swapchain_info.surface		= vulkan->surface;
	swapchain_info.minImageCount	= num_images;
	swapchain_info.imageFormat	= chosen_format.format;
	swapchain_info.imageColorSpace	= chosen_format.colorSpace;
	swapchain_info.imageExtent	= vulkan->swapchain_extent;
	swapchain_info.imageArrayLayers	= 1;
	swapchain_info.imageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.preTransform	= capabilities.currentTransform;
	swapchain_info.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode	= chosen_mode;
	swapchain_info.clipped		= VK_TRUE;
	swapchain_info.oldSwapchain	= old_swapchain;

	const uint32_t queue_family_indices[] = { vulkan->graphics_family_index,
						vulkan->present_family_index };
	if (vulkan->graphics_family_index == vulkan->present_family_index)
	{
		swapchain_info.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount	= 0;
		swapchain_info.pQueueFamilyIndices	= NULL;
	}
	else
	{
		swapchain_info.imageSharingMode		= VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount	= 2;
		swapchain_info.pQueueFamilyIndices	= queue_family_indices;
	}

	if (vkCreateSwapchainKHR(vulkan->device, &swapchain_info, NULL,
				&(vulkan->swapchain)) != VK_SUCCESS)
	{
		vulkan->swapchain = old_swapchain;
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH, "Could not create swapchain.");
		return -1;
	}

	if (old_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vulkan->device, old_swapchain, NULL);
	}

	/* Calls to free() for swapchain images and image views are
	 * in vka_vulkan_shutdown() and the start of this function. */

	if (vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain,
		&(vulkan->num_swapchain_images), NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get number of swapchain images.");
		return -1;
	}

	vulkan->swapchain_images = malloc(vulkan->num_swapchain_images * sizeof(VkImage));
	if (!vulkan->swapchain_images)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for swapchain images.");
		return -1;
	}
	for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
	{
		vulkan->swapchain_images[i] = VK_NULL_HANDLE;
	}

	if (vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain,
		&(vulkan->num_swapchain_images), vulkan->swapchain_images) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get swapchain images.");
		return -1;
	}

	vulkan->swapchain_image_views = malloc(vulkan->num_swapchain_images * sizeof(VkImageView));
	if (!vulkan->swapchain_image_views)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for swapchain image views.");
		return -1;
	}
	for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
	{
		vulkan->swapchain_image_views[i] = VK_NULL_HANDLE;
	}

	for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
	{
		VkImageViewCreateInfo view_info;
		memset(&view_info, 0, sizeof(view_info));
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext					= NULL;
		view_info.flags					= 0;
		view_info.image					= vulkan->swapchain_images[i];
		view_info.viewType				= VK_IMAGE_VIEW_TYPE_2D;
		view_info.format				= vulkan->swapchain_format;
		view_info.components.r				= VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.g				= VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.b				= VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.components.a				= VK_COMPONENT_SWIZZLE_IDENTITY;
		view_info.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel		= 0;
		view_info.subresourceRange.levelCount		= 1;
		view_info.subresourceRange.baseArrayLayer	= 0;
		view_info.subresourceRange.layerCount		= 1;

		if (vkCreateImageView(vulkan->device, &view_info, NULL,
			&(vulkan->swapchain_image_views[i])) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
				"Could not create swapchain image view.");
			return -1;
		}
	}

	if (old_format != vulkan->swapchain_format) { vulkan->recreate_pipelines = 1; }

	return 0;
}

/***********
 * Utility *
 ***********/

void vka_device_wait_idle(vka_vulkan_t *vulkan)
{
	if (vulkan->device == VK_NULL_HANDLE) { return; }
	vkDeviceWaitIdle(vulkan->device);
}

int vka_get_next_swapchain_image(vka_vulkan_t *vulkan)
{
	VkResult error = vkAcquireNextImageKHR(vulkan->device, vulkan->swapchain, UINT64_MAX,
		vulkan->image_available[vulkan->current_frame], VK_NULL_HANDLE,
		&(vulkan->current_swapchain_index));

	if (error)
	{
		if ((error == VK_SUBOPTIMAL_KHR) || (error == VK_ERROR_OUT_OF_DATE_KHR))
		{
			vulkan->recreate_swapchain = 1;
		}
		else
		{
			snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
				"Could not acquire next swapchain image.");
			return -1;
		}
	}

	return 0;
}

/*********
 * Debug *
 *********/

#ifdef VKA_DEBUG
int vka_check_instance_layer_extension_support(vka_vulkan_t *vulkan)
{
	uint32_t num_layers;
	if (vkEnumerateInstanceLayerProperties(&num_layers, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not enumerate instance layers.");
		return -1;
	}

	VkLayerProperties *layers = malloc(num_layers * sizeof(VkLayerProperties));
	if (!layers)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for instance layer properties.");
		return -1;
	}
	if (vkEnumerateInstanceLayerProperties(&num_layers, layers) != VK_SUCCESS)
	{
		free(layers);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH, "Could not get instance layers.");
		return -1;
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
	if (!layer_supported)
	{
		free(layers);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Required layer \"VK_LAYER_KHRONOS_validation\" not supported.");
		return -1;
	}

	free(layers);

	uint32_t num_extensions;
	if (vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not enumerate instance extensions.");
		return -1;
	}

	VkExtensionProperties *extensions = malloc(num_extensions * sizeof(VkExtensionProperties));
	if (!extensions)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not allocate memory for instance extension properties.");
		return -1;
	}
	if (vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not get instance extensions.");
		return -1;
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
	if (!extension_supported)
	{
		free(extensions);
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Required extension \"%s\" not supported.",
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		return -1;
	}

	free(extensions);

	return 0;
}

int vka_create_debug_messenger(vka_vulkan_t *vulkan)
{
	VkDebugUtilsMessengerCreateInfoEXT debug_info;
	memset(&debug_info, 0, sizeof(debug_info));
	debug_info.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.pNext		= NULL;
	debug_info.flags		= 0;
	debug_info.messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
					  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback	= &debug_util_callback;
	debug_info.pUserData		= NULL;

	if (vkCreateDebugUtilsMessengerEXT(vulkan->instance, &debug_info,
			NULL, &(vulkan->debug_messenger)) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, MAX_ERROR_LENGTH,
			"Could not create debug messenger.");
		return -1;
	}

	return 0;
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

void vka_vulkan_print(FILE *file, vka_vulkan_t *vulkan)
{
	fprintf(file, "**************************\n");
	fprintf(file, "* Vulkan base debug info *\n");
	fprintf(file, "**************************\n");

	if (vulkan->config.enabled_features.samplerAnisotropy == VK_TRUE)
	{
		fprintf(file, "Sampler anisotropy\t\t\tEnabled\n");
	}
	else { fprintf(file, "Sampler anisotropy\t\t\tNot enabled\n"); }

	fprintf(file, "Max memory allocation size\t\t= %luMB\n",
		(vulkan->config.max_memory_allocation_size / (1024 * 1024)));

	fprintf(file, "\n");

	if (vulkan->instance == VK_NULL_HANDLE)
	{
		fprintf(file, "Instance\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Instance\t\t\t\t= %p\n", vulkan->instance); }

	if (vulkan->surface == VK_NULL_HANDLE)
	{
		fprintf(file, "Surface\t\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Surface\t\t\t\t\t= %p\n", vulkan->surface); }

	fprintf(file, "\n");

	if (vulkan->physical_device == VK_NULL_HANDLE)
	{
		fprintf(file, "Physical device\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else
	{
		fprintf(file, "Physical device\t\t\t\t= %p\n", vulkan->physical_device);

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(vulkan->physical_device, &properties);
		fprintf(file, "Device selected\t\t\t\t= %s (%d.%d.%d)\n", properties.deviceName,
			VK_API_VERSION_MAJOR(properties.apiVersion),
			VK_API_VERSION_MINOR(properties.apiVersion),
			VK_API_VERSION_PATCH(properties.apiVersion));
	}

	if (vulkan->device == VK_NULL_HANDLE)
	{
		fprintf(file, "Device\t\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Device\t\t\t\t\t= %p\n", vulkan->device); }

	fprintf(file, "\n");

	if (vulkan->graphics_queue == VK_NULL_HANDLE)
	{
		fprintf(file, "Queue: graphics\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Queue: graphics\t\t\t\t= %p\n", vulkan->graphics_queue); }

	if (vulkan->present_queue == VK_NULL_HANDLE)
	{
		fprintf(file, "Queue: present\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Queue: present\t\t\t\t= %p\n", vulkan->present_queue); }

	fprintf(file, "\n");

	if (vulkan->command_pool == VK_NULL_HANDLE)
	{
		fprintf(file, "Command pool\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Command pool\t\t\t\t= %p\n", vulkan->command_pool); }

	fprintf(file, "Command buffers:\n");
	for (uint32_t i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> Command buffer %d\t\t\t= ", i);
		if (vulkan->command_buffers[i] == VK_NULL_HANDLE)
		{
			fprintf(file, "VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "%p\n", vulkan->command_buffers[i]); }
	}

	fprintf(file, "Command fences:\n");
	for (uint32_t i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> Command fence %d\t\t\t= ", i);
		if (vulkan->command_fences[i] == VK_NULL_HANDLE)
		{
			fprintf(file, "VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "%p\n", vulkan->command_fences[i]); }
	}

	fprintf(file, "\n");

	fprintf(file, "Semaphores:\n");

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> \"Image available\" %d\t\t", i);
		if (vulkan->image_available[i] == VK_NULL_HANDLE)
		{
			fprintf(file, "= VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "= %p\n", vulkan->image_available[i]); }
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> \"Render complete\" %d\t\t", i);
		if (vulkan->render_complete[i] == VK_NULL_HANDLE)
		{
			fprintf(file, "= VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "= %p\n", vulkan->render_complete[i]); }
	}

	fprintf(file, "\n");

	fprintf(file, "Swapchain: number of images\t\t= %d\n", vulkan->num_swapchain_images);

	if (vulkan->swapchain == VK_NULL_HANDLE)
	{
		fprintf(file, "Swapchain\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Swapchain\t\t\t\t= %p\n", vulkan->swapchain); }

	if (!vulkan->swapchain_images)
	{
		fprintf(file, "Swapchain images\t\t\t= NULL\n");
	}
	else
	{
		fprintf(file, "Swapchain images\t\t\t= %p\n", vulkan->swapchain_images);
		for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
		{
			fprintf(file, " ---> Swapchain image %d\t\t\t= ", i);
			if (vulkan->swapchain_images[i] == VK_NULL_HANDLE)
			{
				fprintf(file, "VK_NULL_HANDLE\n");
			}
			else
			{
				fprintf(file, "%p\n", vulkan->swapchain_images[i]);
			}
		}
	}

	if (!vulkan->swapchain_image_views)
	{
		fprintf(file, "Swapchain image views\t\t\t= NULL\n");
	}
	else
	{
		fprintf(file, "Swapchain image views\t\t\t= %p\n", vulkan->swapchain_image_views);
		for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
		{
			fprintf(file, " ---> Swapchain image view %d\t\t= ", i);
			if (vulkan->swapchain_image_views[i] == VK_NULL_HANDLE)
			{
				fprintf(file, "VK_NULL_HANDLE\n");
			}
			else
			{
				fprintf(file, "%p\n", vulkan->swapchain_image_views[i]);
			}
		}
	}

	fprintf(file, "\n");

	if (vulkan->debug_messenger == VK_NULL_HANDLE)
	{
		fprintf(file, "Debug messenger\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Debug messenger\t\t\t\t= %p\n", vulkan->debug_messenger); }
}
#endif
