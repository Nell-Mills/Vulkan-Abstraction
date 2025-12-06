#include "Vulkan-Abstraction.h"

/********************
 * Main Vulkan base *
 ********************/

int vka_setup_vulkan(vka_vulkan_t *vulkan)
{
	// Set up default values and enable default features:
	if (!strcmp(vulkan->name, "")) { strcpy(vulkan->name, "Vulkan Application"); }
	if (vulkan->config.minimum_width < 640) { vulkan->config.minimum_width = 640; }
	if (vulkan->config.minimum_height < 480) { vulkan->config.minimum_height = 480; }

	vulkan->config.enabled_features.multiDrawIndirect = VK_TRUE;

	vulkan->config.enabled_features_11.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	vulkan->config.enabled_features_12.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan->config.enabled_features_12.pNext = &(vulkan->config.enabled_features_11);
	vulkan->config.enabled_features_12.descriptorIndexing = VK_TRUE;
	vulkan->config.enabled_features_12.drawIndirectCount = VK_TRUE;
	vulkan->config.enabled_features_12.runtimeDescriptorArray = VK_TRUE;

	vulkan->config.enabled_features_13.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan->config.enabled_features_13.pNext = &(vulkan->config.enabled_features_12);
	vulkan->config.enabled_features_13.dynamicRendering = VK_TRUE;

	vulkan->config.max_memory_allocation_size = 1073741824;
	vulkan->config.max_sampler_descriptors = 1024;
	vulkan->graphics_family_index = 100;
	vulkan->present_family_index = 100;
	vulkan->swapchain_format = VK_FORMAT_UNDEFINED;

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

void vka_shutdown_vulkan(vka_vulkan_t *vulkan)
{
	vka_device_wait_idle(vulkan);

	if (vulkan->swapchain_image_views)
	{
		for (uint32_t i = 0; i < vulkan->num_swapchain_images; i++)
		{
			if (vulkan->swapchain_image_views[i])
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

	if (vulkan->swapchain)
	{
		vkDestroySwapchainKHR(vulkan->device, vulkan->swapchain, NULL);
		vulkan->swapchain = VK_NULL_HANDLE;
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vulkan->image_available[i])
		{
			vkDestroySemaphore(vulkan->device, vulkan->image_available[i], NULL);
			vulkan->image_available[i] = VK_NULL_HANDLE;
		}
		if (vulkan->render_complete[i])
		{
			vkDestroySemaphore(vulkan->device, vulkan->render_complete[i], NULL);
			vulkan->render_complete[i] = VK_NULL_HANDLE;
		}
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vulkan->command_fences[i])
		{
			vkDestroyFence(vulkan->device, vulkan->command_fences[i], NULL);
			vulkan->command_fences[i] = VK_NULL_HANDLE;
		}
	}

	if (vulkan->command_pool)
	{
		vkDestroyCommandPool(vulkan->device, vulkan->command_pool, NULL);
		vulkan->command_pool = VK_NULL_HANDLE;
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vulkan->command_buffers[i] = VK_NULL_HANDLE;
	}

	if (vulkan->device)
	{
		vkDestroyDevice(vulkan->device, NULL);
		vulkan->device = VK_NULL_HANDLE;
	}

	#ifdef VKA_DEBUG
	if (vulkan->debug_messenger)
	{
		vkDestroyDebugUtilsMessengerEXT(vulkan->instance, vulkan->debug_messenger, NULL);
		vulkan->debug_messenger = VK_NULL_HANDLE;
	}
	#endif

	if (vulkan->surface)
	{
		vkDestroySurfaceKHR(vulkan->instance, vulkan->surface, NULL);
		vulkan->surface = VK_NULL_HANDLE;
	}

	if (vulkan->instance)
	{
		vkDestroyInstance(vulkan->instance, NULL);
		vulkan->instance = VK_NULL_HANDLE;
	}

	if (vulkan->window) { SDL_DestroyWindow(vulkan->window); }
	vulkan->window = NULL;
	SDL_Quit();
}

int vka_create_window(vka_vulkan_t *vulkan)
{
	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not initialise SDL -> %s.", SDL_GetError());
		return -1;
	}

	vulkan->window = SDL_CreateWindow(vulkan->name,
		vulkan->config.minimum_width, vulkan->config.minimum_height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!vulkan->window)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not create window -> %s.", SDL_GetError());
		return -1;
	}

	if (SDL_SetWindowFullscreenMode(vulkan->window, NULL) != true)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not set window fullscreen mode -> %s.", SDL_GetError());
		return -1;
	}

	if (SDL_SetWindowMinimumSize(vulkan->window, vulkan->config.minimum_width,
					vulkan->config.minimum_height) != true)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
	application_info.pApplicationName	= vulkan->name;
	application_info.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName		= vulkan->name;
	application_info.engineVersion		= VK_MAKE_VERSION(1, 0, 0);
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for extension names.");
		return -1;
	}
	for (uint32_t i = 0; i < extension_count; i++)
	{
		extensions[i] = malloc(NM_MAX_NAME_LENGTH * sizeof(char));
		if (!extensions[i])
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not allocate memory for extension names.");
			for (uint32_t j = 0; j < i; j++) { free(extensions[j]); }
			free(extensions);
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not create Vulkan instance.");
		vulkan->instance = VK_NULL_HANDLE;
		for (uint32_t i = 0; i < extension_count; i++) { free(extensions[i]); }
		free(extensions);
		return -1;
	}

	volkLoadInstanceOnly(vulkan->instance);

	for (uint32_t i = 0; i < extension_count; i++) { free(extensions[i]); }
	free(extensions);
	return 0;
}

int vka_create_surface(vka_vulkan_t *vulkan)
{
	if (SDL_Vulkan_CreateSurface(vulkan->window, vulkan->instance,
				NULL, &(vulkan->surface)) != true)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not enumerate physical devices.");
		return -1;
	}

	VkPhysicalDevice *physical_devices = malloc(num_physical_devices *
						sizeof(VkPhysicalDevice));
	if (!physical_devices)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for physical device array.");
		return -1;
	}

	if (vkEnumeratePhysicalDevices(vulkan->instance, &num_physical_devices,
					physical_devices) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get array of physical devices.");
		free(physical_devices);
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH, "Could not create device.");
		free(queue_info);
		return -1;
	}

	free(queue_info);

	volkLoadDevice(vulkan->device);

	vkGetDeviceQueue(vulkan->device, vulkan->graphics_family_index, 0,
					&(vulkan->graphics_queue));
	if (!vulkan->graphics_queue)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not retrieve graphics queue.");
		return -1;
	}

	vkGetDeviceQueue(vulkan->device, vulkan->present_family_index, 0,
					&(vulkan->present_queue));
	if (!vulkan->present_queue)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not enumerate extensions for physical device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	VkExtensionProperties *supported_extensions = malloc(num_supported_extensions *
							sizeof(VkExtensionProperties));
	if (!supported_extensions)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for extensions for physical device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	if (vkEnumerateDeviceExtensionProperties(physical_device, NULL,
		&num_supported_extensions, supported_extensions) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get array of supported extensions for physical device: %s.",
			get_properties.properties.deviceName);
		free(supported_extensions);
		return -1;
	}

	int supported = 0;
	for (uint32_t i = 0; i < num_supported_extensions; i++)
	{
		if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, supported_extensions[i].extensionName))
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for feature comparison for device: %s.",
			get_properties.properties.deviceName);
		return -1;
	}

	uint8_t *supported_features_array = malloc(total_size);
	if (!supported_features_array)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for feature comparison for device: %s.",
			get_properties.properties.deviceName);
		free(desired_features_array);
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
			free(supported_features_array);
			free(desired_features_array);
			return -1;
		}
	}

	free(supported_features_array);
	free(desired_features_array);

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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not create command pool.");
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
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not create semaphore: \"Image available\".");
			return -1;
		}

		if (vkCreateSemaphore(vulkan->device, &semaphore_info, NULL,
			&(vulkan->render_complete[i])) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
			if (vulkan->swapchain_image_views[i])
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not enumerate surface formats.");
		return -1;
	}

	VkSurfaceFormatKHR *formats = malloc(num_formats * sizeof(VkSurfaceFormatKHR));
	if (!formats)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for array of surface formats.");
		return -1;
	}

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->physical_device, vulkan->surface,
						&num_formats, formats) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get array of surface formats.");
		free(formats);
		return -1;
	}

	if (num_formats < 1)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"No surface formats to choose from.");
		free(formats);
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not enumerate surface present modes.");
		return -1;
	}

	VkPresentModeKHR *modes = malloc(num_modes * sizeof(VkPresentModeKHR));
	if (!modes)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for array of surface present modes.");
		return -1;
	}

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->physical_device, vulkan->surface,
							&num_modes, modes) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get array of surface present modes.");
		free(modes);
		return -1;
	}

	if (num_modes < 1)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"No surface present modes to choose from.");
		free(modes);
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH, "Could not create swapchain.");
		vulkan->swapchain = old_swapchain;
		return -1;
	}

	if (old_swapchain) { vkDestroySwapchainKHR(vulkan->device, old_swapchain, NULL); }

	/* Calls to free() for swapchain images and image views are
	 * in vka_vulkan_shutdown() and the start of this function. */

	if (vkGetSwapchainImagesKHR(vulkan->device, vulkan->swapchain,
		&(vulkan->num_swapchain_images), NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get number of swapchain images.");
		return -1;
	}

	vulkan->swapchain_images = malloc(vulkan->num_swapchain_images * sizeof(VkImage));
	if (!vulkan->swapchain_images)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not get swapchain images.");
		return -1;
	}

	vulkan->swapchain_image_views = malloc(vulkan->num_swapchain_images * sizeof(VkImageView));
	if (!vulkan->swapchain_image_views)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not create swapchain image view.");
			return -1;
		}
	}

	if (old_format != vulkan->swapchain_format) { vulkan->recreate_pipelines = 1; }

	return 0;
}

/*************
 * Pipelines *
 *************/

int vka_create_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline)
{
	if (!pipeline->layout)
	{
		VkPipelineLayoutCreateInfo pipeline_layout_info;
		memset(&pipeline_layout_info, 0, sizeof(pipeline_layout_info));
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.pNext			= NULL;
		pipeline_layout_info.flags			= 0;
		pipeline_layout_info.setLayoutCount = pipeline->config.num_descriptor_set_layouts;
		pipeline_layout_info.pSetLayouts = pipeline->config.descriptor_set_layouts;
		pipeline_layout_info.pushConstantRangeCount	= 0;
		pipeline_layout_info.pPushConstantRanges	= NULL;

		if (vkCreatePipelineLayout(vulkan->device, &pipeline_layout_info,
					NULL, &(pipeline->layout)) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not create pipeline layout for \"%s\".", pipeline->name);
			return -1;
		}
	}

	// Check for compute pipeline:
	if (pipeline->config.is_compute_pipeline)
	{
		vka_shader_t *c_shader = &(pipeline->shaders[VKA_SHADER_TYPE_COMPUTE]);
		if (!c_shader->shader && vka_create_shader_module(vulkan, c_shader)) { return -1; }

		VkPipelineShaderStageCreateInfo c_shader_stage_info;
		memset(&c_shader_stage_info, 0, sizeof(c_shader_stage_info));
		c_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		c_shader_stage_info.pNext		= NULL;
		c_shader_stage_info.flags		= 0;
		c_shader_stage_info.stage		= VK_SHADER_STAGE_COMPUTE_BIT;
		c_shader_stage_info.module		= c_shader->shader;
		c_shader_stage_info.pName		= "main";
		c_shader_stage_info.pSpecializationInfo	= NULL;

		VkComputePipelineCreateInfo c_pipeline_info;
		memset(&c_pipeline_info, 0, sizeof(c_pipeline_info));
		c_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		c_pipeline_info.pNext			= NULL;
		c_pipeline_info.flags			= 0;
		c_pipeline_info.stage			= c_shader_stage_info;
		c_pipeline_info.layout			= pipeline->layout;
		c_pipeline_info.basePipelineHandle	= NULL;
		c_pipeline_info.basePipelineIndex	= 0;

		if (vkCreateComputePipelines(vulkan->device, VK_NULL_HANDLE, 1, &c_pipeline_info,
						NULL, &(pipeline->pipeline)) != VK_SUCCESS)
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not create compute pipeline \"%s\".", pipeline->name);
			return -1;
		}

		return 0;
	}

	// Validate config values:
	for (uint32_t i = 0; i < pipeline->config.num_vertex_bindings; i++)
	{
		if (pipeline->config.strides[i] == 0)
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Vertex binding stride %u for pipeline \"%s\" is zero.",
				i, pipeline->name);
			return -1;
		}
	}
	for (uint32_t i = 0; i < pipeline->config.num_vertex_attributes; i++)
	{
		if (pipeline->config.bindings[i] >= pipeline->config.num_vertex_bindings)
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Vertex attribute binding %u for pipeline \"%s\" is out of bounds.",
				i, pipeline->name);
			return -1;
		}
		if (pipeline->config.formats[i] == VK_FORMAT_UNDEFINED)
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Vertex attribute %u for pipeline \"%s\" has no format.",
				i, pipeline->name);
			return -1;
		}
	}
	if (pipeline->config.line_width < 1.f) { pipeline->config.line_width = 1.f; }
	// TODO - colour write mask?

	uint32_t stage_count = 2;
	vka_shader_t *shaders[2] = { &(pipeline->shaders[VKA_SHADER_TYPE_VERTEX]),
					&(pipeline->shaders[VKA_SHADER_TYPE_FRAGMENT]) };
	VkShaderStageFlagBits stage_flags[2] = { VK_SHADER_STAGE_VERTEX_BIT,
						VK_SHADER_STAGE_FRAGMENT_BIT };

	VkPipelineShaderStageCreateInfo shader_stages[stage_count];
	memset(shader_stages, 0, stage_count * sizeof(shader_stages[0]));
	for (uint32_t i = 0; i < stage_count; i++)
	{
		if (!shaders[i]->shader && vka_create_shader_module(vulkan, shaders[i]))
		{
			return -1;
		}

		shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[i].pNext			= NULL;
		shader_stages[i].flags			= 0;
		shader_stages[i].stage			= stage_flags[i];
		shader_stages[i].module			= shaders[i]->shader;
		shader_stages[i].pName			= "main";
		shader_stages[i].pSpecializationInfo	= NULL;
	}

	// Vertex input:
	VkVertexInputBindingDescription vertex_bindings[VKA_MAX_VERTEX_ATTRIBUTES];
	memset(vertex_bindings, 0, VKA_MAX_VERTEX_ATTRIBUTES * sizeof(vertex_bindings[0]));
	for (uint32_t i = 0; i < pipeline->config.num_vertex_bindings; i++)
	{
		vertex_bindings[i].binding	= i;
		vertex_bindings[i].stride	= pipeline->config.strides[i];
		vertex_bindings[i].inputRate	= VK_VERTEX_INPUT_RATE_VERTEX;
	}

	VkVertexInputAttributeDescription vertex_attributes[VKA_MAX_VERTEX_ATTRIBUTES];
	memset(vertex_attributes, 0, VKA_MAX_VERTEX_ATTRIBUTES * sizeof(vertex_attributes[0]));
	for (uint32_t i = 0; i < pipeline->config.num_vertex_attributes; i++)
	{
		vertex_attributes[i].location	= i;
		vertex_attributes[i].binding	= pipeline->config.bindings[i];
		vertex_attributes[i].format	= pipeline->config.formats[i];
		vertex_attributes[i].offset	= pipeline->config.offsets[i];
	}

	VkPipelineVertexInputStateCreateInfo input_info;
	memset(&input_info, 0, sizeof(input_info));
	input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	input_info.pNext				= NULL;
	input_info.flags				= 0;
	input_info.vertexBindingDescriptionCount	= pipeline->config.num_vertex_bindings;
	input_info.pVertexBindingDescriptions		= vertex_bindings;
	input_info.vertexAttributeDescriptionCount	= pipeline->config.num_vertex_attributes;
	input_info.pVertexAttributeDescriptions		= vertex_attributes;

	// Input assembly:
	VkPipelineInputAssemblyStateCreateInfo assembly_info;
	memset(&assembly_info, 0, sizeof(assembly_info));
	assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly_info.pNext			= NULL;
	assembly_info.flags			= 0;
	assembly_info.primitiveRestartEnable	= VK_FALSE;
	assembly_info.topology			= pipeline->config.topology;

	// Viewport (will be using dynamic state):
	VkViewport viewport;
	memset(&viewport, 0, sizeof(viewport));
	VkRect2D scissor;
	memset(&scissor, 0, sizeof(scissor));

	VkPipelineViewportStateCreateInfo viewport_info;
	memset(&viewport_info, 0, sizeof(viewport_info));
	viewport_info.sType		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.pNext		= NULL;
	viewport_info.flags		= 0;
	viewport_info.viewportCount	= 1;
	viewport_info.pViewports	= &viewport;
	viewport_info.scissorCount	= 1;
	viewport_info.pScissors		= &scissor;

	// Rasterisation:
	VkPipelineRasterizationStateCreateInfo rasterization_info;
	memset(&rasterization_info, 0, sizeof(rasterization_info));
	rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_info.pNext			= NULL;
	rasterization_info.flags			= 0;
	rasterization_info.depthClampEnable		= VK_FALSE;
	rasterization_info.rasterizerDiscardEnable	= VK_FALSE; // TODO - check this.
	rasterization_info.polygonMode			= VK_POLYGON_MODE_FILL;
	rasterization_info.cullMode			= pipeline->config.cull_mode;
	rasterization_info.frontFace			= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_info.depthBiasEnable		= VK_FALSE;
	rasterization_info.depthBiasConstantFactor	= 0.f;
	rasterization_info.depthBiasClamp		= 0.f;
	rasterization_info.depthBiasSlopeFactor		= 0.f;
	rasterization_info.lineWidth			= pipeline->config.line_width;

	// Multisampling (TODO):
	VkPipelineMultisampleStateCreateInfo multisample_info;
	memset(&multisample_info, 0, sizeof(multisample_info));
	multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.pNext			= NULL;
	multisample_info.flags			= 0;
	multisample_info.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
	multisample_info.sampleShadingEnable	= VK_FALSE;
	multisample_info.minSampleShading	= 0.f;
	multisample_info.pSampleMask		= NULL;
	multisample_info.alphaToCoverageEnable	= VK_FALSE;
	multisample_info.alphaToOneEnable	= VK_FALSE;

	// Depth stencil (TODO):
	VkPipelineDepthStencilStateCreateInfo depth_info;
	memset(&depth_info, 0, sizeof(depth_info));
	depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_info.pNext			= NULL;
	depth_info.flags			= 0;
	depth_info.depthTestEnable		= VK_FALSE;
	depth_info.depthWriteEnable		= VK_FALSE;
	depth_info.depthCompareOp		= VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_info.depthBoundsTestEnable	= VK_FALSE;
	depth_info.stencilTestEnable		= VK_FALSE;

	depth_info.front.failOp			= VK_STENCIL_OP_KEEP;
	depth_info.front.passOp			= VK_STENCIL_OP_KEEP;
	depth_info.front.depthFailOp		= VK_STENCIL_OP_KEEP;
	depth_info.front.compareOp		= VK_COMPARE_OP_NEVER;
	depth_info.front.compareMask		= 0;
	depth_info.front.writeMask		= 0;
	depth_info.front.reference		= 0;

	depth_info.back.failOp			= VK_STENCIL_OP_KEEP;
	depth_info.back.passOp			= VK_STENCIL_OP_KEEP;
	depth_info.back.depthFailOp		= VK_STENCIL_OP_KEEP;
	depth_info.back.compareOp		= VK_COMPARE_OP_NEVER;
	depth_info.back.compareMask		= 0;
	depth_info.back.writeMask		= 0;
	depth_info.back.reference		= 0;

	depth_info.minDepthBounds		= 0.f;
	depth_info.maxDepthBounds		= 1.f;

	// Blend:
	VkPipelineColorBlendAttachmentState blend_state;
	memset(&blend_state, 0, sizeof(blend_state));
	blend_state.blendEnable		= pipeline->config.blend_enable;
	blend_state.srcColorBlendFactor	= VK_BLEND_FACTOR_ZERO;
	blend_state.dstColorBlendFactor	= VK_BLEND_FACTOR_ZERO;
	blend_state.colorBlendOp	= pipeline->config.colour_blend_op;
	blend_state.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
	blend_state.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
	blend_state.alphaBlendOp	= pipeline->config.alpha_blend_op;
	blend_state.colorWriteMask	= pipeline->config.colour_write_mask;

	VkPipelineColorBlendStateCreateInfo blend_info;
	memset(&blend_info, 0, sizeof(blend_info));
	blend_info.sType		= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_info.pNext		= NULL;
	blend_info.flags		= 0;
	blend_info.logicOpEnable	= VK_FALSE;
	blend_info.logicOp		= VK_LOGIC_OP_CLEAR;
	blend_info.attachmentCount	= 1;
	blend_info.pAttachments		= &blend_state;
	blend_info.blendConstants[0]	= 0.f;
	blend_info.blendConstants[1]    = 0.f;
	blend_info.blendConstants[2]    = 0.f;
	blend_info.blendConstants[3]	= 0.f;

	// Dynamic state:
	VkDynamicState dynamic_state[2];
	dynamic_state[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamic_state[1] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDynamicStateCreateInfo dynamic_state_info;
	memset(&dynamic_state_info, 0, sizeof(dynamic_state_info));
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.pNext		= NULL;
	dynamic_state_info.flags		= 0;
	dynamic_state_info.dynamicStateCount	= 2;
	dynamic_state_info.pDynamicStates	= dynamic_state;

	// Rendering:
	VkPipelineRenderingCreateInfo pipeline_rendering_info;
	memset(&pipeline_rendering_info, 0, sizeof(pipeline_rendering_info));
	pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_info.pNext			= NULL;
	pipeline_rendering_info.viewMask		= 0;
	pipeline_rendering_info.colorAttachmentCount	= 1;
	pipeline_rendering_info.pColorAttachmentFormats	= &(vulkan->swapchain_format);
	pipeline_rendering_info.depthAttachmentFormat	= VK_FORMAT_UNDEFINED; // TODO.
	pipeline_rendering_info.stencilAttachmentFormat	= VK_FORMAT_UNDEFINED;

	// Create pipeline:
	VkGraphicsPipelineCreateInfo pipeline_info;
	memset(&pipeline_info, 0, sizeof(pipeline_info));
	pipeline_info.sType			= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext			= &pipeline_rendering_info;
	pipeline_info.flags			= 0;
	pipeline_info.stageCount		= stage_count;
	pipeline_info.pStages			= shader_stages;
	pipeline_info.pVertexInputState		= &input_info;
	pipeline_info.pInputAssemblyState	= &assembly_info;
	pipeline_info.pTessellationState	= NULL;
	pipeline_info.pViewportState		= &viewport_info;
	pipeline_info.pRasterizationState	= &rasterization_info;
	pipeline_info.pMultisampleState		= &multisample_info;
	pipeline_info.pDepthStencilState	= &depth_info;
	pipeline_info.pColorBlendState		= &blend_info;
	pipeline_info.pDynamicState		= &dynamic_state_info;
	pipeline_info.layout			= pipeline->layout;
	pipeline_info.renderPass		= VK_NULL_HANDLE;
	pipeline_info.subpass			= 0;
	pipeline_info.basePipelineHandle	= VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex		= 0;

	if (vkCreateGraphicsPipelines(vulkan->device, VK_NULL_HANDLE, 1,
		&pipeline_info, NULL, &(pipeline->pipeline)) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not create pipeline \"%s\".", pipeline->name);
		return -1;
	}

	return 0;
}

void vka_destroy_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline)
{
	if (pipeline->pipeline)
	{
		vkDestroyPipeline(vulkan->device, pipeline->pipeline, NULL);
		pipeline->pipeline = VK_NULL_HANDLE;
	}

	for (int i = 0; i < 3; i++) { vka_destroy_shader_module(vulkan, &(pipeline->shaders[i])); }

	if (pipeline->layout)
	{
		vkDestroyPipelineLayout(vulkan->device, pipeline->layout, NULL);
		pipeline->layout = VK_NULL_HANDLE;
	}
}

void vka_bind_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline)
{
	VkPipelineBindPoint bind_point;
	if (pipeline->shaders[VKA_SHADER_TYPE_COMPUTE].shader)
	{
		bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
	}
	else { bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS; }
	vkCmdBindPipeline(vulkan->command_buffers[vulkan->current_frame],
					bind_point, pipeline->pipeline);
}

/***********
 * Shaders *
 ***********/

int vka_create_shader_module(vka_vulkan_t *vulkan, vka_shader_t *shader)
{
	FILE *shader_file = fopen(shader->path, "rb");
	if (!shader_file)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not open shader file \"%s\".", shader->path);
		return -1;
	}

	if (fseek(shader_file, 0, SEEK_END))
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not navigate shader file \"%s\".", shader->path);
		fclose(shader_file);
		return -1;
	}
	long file_size = ftell(shader_file);
	if (file_size == -1)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get size of shader file \"%s\".", shader->path);
		fclose(shader_file);
		return -1;
	}
	if (file_size % 4)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Shader file \"%s\" size is not a multiple of 4 bytes.", shader->path);
		fclose(shader_file);
		return -1;
	}
	rewind(shader_file);

	size_t code_size = file_size;
	uint32_t *shader_code = malloc(code_size);
	if (!shader_code)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for shader code from file \"%s\".",
			shader->path);
		fclose(shader_file);
		return -1;
	}

	if (fread(shader_code, 4, code_size / 4, shader_file) != (code_size / 4))
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not read shader code from file \"%s\".", shader->path);
		free(shader_code);
		fclose(shader_file);
		return -1;
	}

	fclose(shader_file);

	VkShaderModuleCreateInfo shader_module_info;
	memset(&shader_module_info, 0, sizeof(shader_module_info));
	shader_module_info.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.pNext	= NULL;
	shader_module_info.flags	= 0;
	shader_module_info.codeSize	= code_size;
	shader_module_info.pCode	= shader_code;

	// Create a temporary shader module in case this doesn't work out:
	VkShaderModule temp = VK_NULL_HANDLE;
	if (vkCreateShaderModule(vulkan->device, &shader_module_info, NULL, &temp) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not create shader module for \"%s\".", shader->path);
		free(shader_code);
		return -1;
	}

	vka_destroy_shader_module(vulkan, shader);
	shader->shader = temp;

	free(shader_code);
	return 0;
}

void vka_destroy_shader_module(vka_vulkan_t *vulkan, vka_shader_t *shader)
{
	if (shader->shader)
	{
		vkDestroyShaderModule(vulkan->device, shader->shader, NULL);
		shader->shader = VK_NULL_HANDLE;
	}
}

/***********
 * Utility *
 ***********/

void vka_device_wait_idle(vka_vulkan_t *vulkan)
{
	if (vulkan->device) { vkDeviceWaitIdle(vulkan->device); }
}

int vka_get_next_swapchain_image(vka_vulkan_t *vulkan)
{
	VkResult error = vkAcquireNextImageKHR(vulkan->device, vulkan->swapchain, UINT64_MAX,
		vulkan->image_available[vulkan->current_frame], VK_NULL_HANDLE,
		&(vulkan->current_swapchain_index));

	if (error != VK_SUCCESS)
	{
		if ((error == VK_SUBOPTIMAL_KHR) || (error == VK_ERROR_OUT_OF_DATE_KHR))
		{
			vulkan->recreate_swapchain = 1;
		}
		else
		{
			snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not enumerate instance layers.");
		return -1;
	}

	VkLayerProperties *layers = malloc(num_layers * sizeof(VkLayerProperties));
	if (!layers)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for instance layer properties.");
		return -1;
	}
	if (vkEnumerateInstanceLayerProperties(&num_layers, layers) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
				"Could not get instance layers.");
		free(layers);
		return -1;
	}

	int layer_supported = 0;
	for (uint32_t i = 0; i < num_layers; i++)
	{
		if (!strcmp("VK_LAYER_KHRONOS_validation", layers[i].layerName))
		{
			layer_supported = 1;
			break;
		}
	}
	if (!layer_supported)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Required layer \"VK_LAYER_KHRONOS_validation\" not supported.");
		free(layers);
		return -1;
	}

	free(layers);

	uint32_t num_extensions;
	if (vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not enumerate instance extensions.");
		return -1;
	}

	VkExtensionProperties *extensions = malloc(num_extensions * sizeof(VkExtensionProperties));
	if (!extensions)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for instance extension properties.");
		return -1;
	}
	if (vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions) != VK_SUCCESS)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Could not get instance extensions.");
		free(extensions);
		return -1;
	}

	int extension_supported = 0;
	for (uint32_t i = 0; i < num_extensions; i++)
	{
		if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensions[i].extensionName))
		{
			extension_supported = 1;
			break;
		}
	}
	if (!extension_supported)
	{
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
			"Required extension \"%s\" not supported.",
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		free(extensions);
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
		snprintf(vulkan->error_message, NM_MAX_ERROR_LENGTH,
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
	{
		return "SEVERITY_VERBOSE";
	}
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		return "SEVERITY_INFO";
	}
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		return "SEVERITY_WARNING";
	}
	else if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		return "SEVERITY_ERROR";
	}
	else { return "UNKNOWN SEVERITY TYPE"; }
}

char *vka_debug_get_type(VkDebugUtilsMessageTypeFlagsEXT type)
{
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) { return "GENERAL"; }
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) { return "VALIDATION"; }
	if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) { return "PERFORMANCE"; }
	else { return "UNKNOWN MESSAGE TYPE"; }
}

void vka_print_vulkan(FILE *file, vka_vulkan_t *vulkan)
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

	if (!vulkan->instance)
	{
		fprintf(file, "Instance\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Instance\t\t\t\t= %p\n", vulkan->instance); }

	if (!vulkan->surface)
	{
		fprintf(file, "Surface\t\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Surface\t\t\t\t\t= %p\n", vulkan->surface); }

	fprintf(file, "\n");

	if (!vulkan->physical_device)
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

	if (!vulkan->device)
	{
		fprintf(file, "Device\t\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Device\t\t\t\t\t= %p\n", vulkan->device); }

	fprintf(file, "\n");

	if (!vulkan->graphics_queue)
	{
		fprintf(file, "Queue: graphics\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Queue: graphics\t\t\t\t= %p\n", vulkan->graphics_queue); }

	if (!vulkan->present_queue)
	{
		fprintf(file, "Queue: present\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Queue: present\t\t\t\t= %p\n", vulkan->present_queue); }

	fprintf(file, "\n");

	if (!vulkan->command_pool)
	{
		fprintf(file, "Command pool\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Command pool\t\t\t\t= %p\n", vulkan->command_pool); }

	fprintf(file, "Command buffers:\n");
	for (uint32_t i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> Command buffer %d\t\t\t= ", i);
		if (!vulkan->command_buffers[i])
		{
			fprintf(file, "VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "%p\n", vulkan->command_buffers[i]); }
	}

	fprintf(file, "Command fences:\n");
	for (uint32_t i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> Command fence %d\t\t\t= ", i);
		if (!vulkan->command_fences[i])
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
		if (!vulkan->image_available[i])
		{
			fprintf(file, "= VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "= %p\n", vulkan->image_available[i]); }
	}

	for (int i = 0; i < VKA_MAX_FRAMES_IN_FLIGHT; i++)
	{
		fprintf(file, " ---> \"Render complete\" %d\t\t", i);
		if (!vulkan->render_complete[i])
		{
			fprintf(file, "= VK_NULL_HANDLE\n");
		}
		else { fprintf(file, "= %p\n", vulkan->render_complete[i]); }
	}

	fprintf(file, "\n");

	fprintf(file, "Swapchain: number of images\t\t= %d\n", vulkan->num_swapchain_images);

	if (!vulkan->swapchain)
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
			if (!vulkan->swapchain_images[i])
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
			if (!vulkan->swapchain_image_views[i])
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

	if (!vulkan->debug_messenger)
	{
		fprintf(file, "Debug messenger\t\t\t\t= VK_NULL_HANDLE\n");
	}
	else { fprintf(file, "Debug messenger\t\t\t\t= %p\n", vulkan->debug_messenger); }
}

void vka_print_pipeline(FILE *file, vka_pipeline_t *pipeline)
{
	fprintf(file, "******************************\n");
	fprintf(file, "* Vulkan pipeline debug info *\n");
	fprintf(file, "******************************\n");

	fprintf(file, "Pipeline name: %s\n", pipeline->name);
	fprintf(file, "Compute pipeline: ");
	if (pipeline->config.is_compute_pipeline) { fprintf(file, "Yes\n"); }
	else { fprintf(file, "No\n"); }

	fprintf(file, "\n");

	if (!pipeline->layout) { fprintf(file, "Pipeline layout\t\t\t\t= VK_NULL_HANDLE\n"); }
	else { fprintf(file, "Pipeline layout\t\t\t\t= %p\n", pipeline->layout); }

	if (!pipeline->pipeline) { fprintf(file, "Pipeline\t\t\t\t= VK_NULL_HANDLE\n"); }
	else { fprintf(file, "Pipeline\t\t\t\t= %p\n", pipeline->pipeline); }

	char *shader_types[3] = { "Vertex shader:\t\t\t\t",
				"Fragment shader:\t\t\t",
				"Compute shader:\t\t\t\t" };
	for (int i = 0; i < 3; i++)
	{
		if (!pipeline->shaders[i].shader)
		{
			fprintf(file, "%s= VK_NULL_HANDLE\n", shader_types[i]);
		}
		else { fprintf(file, "%s= %p\n", shader_types[i], pipeline->shaders[i].shader); }
	}
}
#endif
