#ifndef VULKAN_ABSTRACTION_H
#define VULKAN_ABSTRACTION_H

#include <NM-Config/Config.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <Volk/volk.h>

#define VKA_API_VERSION_MAJOR 1
#define VKA_API_VERSION_MINOR 3

#ifndef VKA_MAX_FRAMES_IN_FLIGHT
#define VKA_MAX_FRAMES_IN_FLIGHT 2
#endif

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];

	int minimum_width;
	int minimum_height;

	// Feature requirements (after device creation, these represent enabled features):
	VkPhysicalDeviceFeatures enabled_features;
	VkPhysicalDeviceVulkan11Features enabled_features_11;
	VkPhysicalDeviceVulkan12Features enabled_features_12;
	VkPhysicalDeviceVulkan13Features enabled_features_13;

	// Limits:
	VkDeviceSize max_memory_allocation_size;
	int max_sampler_descriptors;
} vka_vulkan_config_t;

typedef struct
{
	vka_vulkan_config_t config;

	SDL_Window *window;
	VkInstance instance;
	VkSurfaceKHR surface;

	VkPhysicalDevice physical_device;
	VkDevice device;
	uint32_t graphics_family_index;
	VkQueue graphics_queue;
	uint32_t present_family_index;
	VkQueue present_queue;

	VkCommandPool command_pool;
	VkCommandBuffer command_buffers[VKA_MAX_FRAMES_IN_FLIGHT];
	VkFence command_fences[VKA_MAX_FRAMES_IN_FLIGHT];

	VkSemaphore image_available[VKA_MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_complete[VKA_MAX_FRAMES_IN_FLIGHT];

	uint32_t num_swapchain_images;
	VkFormat swapchain_format;
	VkExtent2D swapchain_extent;
	VkSwapchainKHR swapchain;
	VkImage *swapchain_images;
	VkImageView *swapchain_image_views;

	uint8_t recreate_swapchain;
	uint8_t recreate_pipelines;
	uint8_t current_frame;
	uint32_t current_swapchain_index;

	char error_message[NM_MAX_ERROR_LENGTH];
	#ifdef VKA_DEBUG
	VkDebugUtilsMessengerEXT debug_messenger;
	#endif
} vka_vulkan_t;

// Main Vulkan base:
int vka_vulkan_setup(vka_vulkan_t *vulkan);
void vka_vulkan_shutdown(vka_vulkan_t *vulkan);
int vka_create_window(vka_vulkan_t *vulkan);
int vka_create_instance(vka_vulkan_t *vulkan);
int vka_create_surface(vka_vulkan_t *vulkan);
int vka_create_device(vka_vulkan_t *vulkan);
int vka_score_physical_device(vka_vulkan_t *vulkan, VkPhysicalDevice physical_device,
	uint32_t *graphics_family_index, uint32_t *present_family_index,
	VkDeviceSize *max_memory_allocation_size, VkBool32 *sampler_anisotropy);
int vka_create_command_pool(vka_vulkan_t *vulkan);
int vka_create_command_buffers(vka_vulkan_t *vulkan);
int vka_create_command_fences(vka_vulkan_t *vulkan);
int vka_create_semaphores(vka_vulkan_t *vulkan);
int vka_create_swapchain(vka_vulkan_t *vulkan);

// Utility:
void vka_device_wait_idle(vka_vulkan_t *vulkan);
int vka_get_next_swapchain_image(vka_vulkan_t *vulkan);

// Debug:
#ifdef VKA_DEBUG
int vka_check_instance_layer_extension_support(vka_vulkan_t *vulkan);
int vka_create_debug_messenger(vka_vulkan_t *vulkan);
VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	VkDebugUtilsMessengerCallbackDataEXT const *data,
	void *user_pointer);
char *vka_debug_get_severity(VkDebugUtilsMessageSeverityFlagBitsEXT severity);
char *vka_debug_get_type(VkDebugUtilsMessageTypeFlagsEXT type);
void vka_vulkan_print(FILE *file, vka_vulkan_t *vulkan);
#endif

#endif
