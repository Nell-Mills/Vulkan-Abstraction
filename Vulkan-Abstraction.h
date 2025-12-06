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

#ifndef VKA_MAX_VERTEX_ATTRIBUTES
#define VKA_MAX_VERTEX_ATTRIBUTES 4
#endif

#define VKA_SHADER_TYPE_VERTEX		0
#define VKA_SHADER_TYPE_FRAGMENT	1
#define VKA_SHADER_TYPE_COMPUTE		2

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkCommandBuffer buffer;
	VkFence fence;

	/*****************
	 * Configuration *
	 *****************/

	uint8_t fence_signaled;
} vka_command_buffer_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
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
	vka_command_buffer_t command_buffers[VKA_MAX_FRAMES_IN_FLIGHT];

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

	/*****************
	 * Configuration *
	 *****************/
	int minimum_window_width;
	int minimum_window_height;

	// Feature requirements (after device creation, these represent enabled features):
	VkPhysicalDeviceFeatures enabled_features;
	VkPhysicalDeviceVulkan11Features enabled_features_11;
	VkPhysicalDeviceVulkan12Features enabled_features_12;
	VkPhysicalDeviceVulkan13Features enabled_features_13;

	// Limits:
	VkDeviceSize max_memory_allocation_size;
	int max_sampler_descriptors;
} vka_vulkan_t;

typedef struct
{
	char path[NM_MAX_PATH_LENGTH];
	VkShaderModule shader;
} vka_shader_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkPipelineLayout layout;
	VkPipeline pipeline;
	vka_shader_t shaders[3];

	/*****************
	 * Configuration *
	 *****************/
	int is_compute_pipeline;

	/* Note - actual descriptor set layouts are managed OUTSIDE the
	 * pipeline, but this array is freed by vka_destroy_pipeline().
	 * Allocate it outside the pipeline as well. */
	uint32_t num_descriptor_set_layouts;
	VkDescriptorSetLayout *descriptor_set_layouts;

	// Vertex input:
	uint32_t num_vertex_bindings;
	uint32_t strides[VKA_MAX_VERTEX_ATTRIBUTES];
	uint32_t num_vertex_attributes;
	uint32_t bindings[VKA_MAX_VERTEX_ATTRIBUTES];
	VkFormat formats[VKA_MAX_VERTEX_ATTRIBUTES];
	uint32_t offsets[VKA_MAX_VERTEX_ATTRIBUTES];

	// Input assembly:
	VkPrimitiveTopology topology;	// Default VK_PRIMITIVE_TOPOLOGY_POINT_LIST.

	// Rasterisation:
	VkPolygonMode polygon_mode;	// Default VK_POLYGON_MODE_FILL.
	VkCullModeFlags cull_mode;	// Default VK_CULL_MODE_NONE.
	float line_width;		// Default and minimum 1.f.

	// TODO multisampling.
	// TODO depth stencil.

	// Blend:
	VkBool32 blend_enable;		// Default VK_FALSE.
	VkBlendOp colour_blend_op;	// Default VK_BLEND_OP_ADD.
	VkBlendOp alpha_blend_op;
	VkColorComponentFlags colour_write_mask; // If 0, gets set to RGBA for graphics pipelines.
} vka_pipeline_t;

// Main Vulkan base:
int vka_setup_vulkan(vka_vulkan_t *vulkan);
void vka_shutdown_vulkan(vka_vulkan_t *vulkan);
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

// Pipelines and shaders:
int vka_create_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline);
void vka_destroy_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline);
void vka_bind_pipeline(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer,
							vka_pipeline_t *pipeline);
int vka_create_shader(vka_vulkan_t *vulkan, vka_shader_t *shader);

// Command buffers and fences:
int vka_create_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
void vka_destroy_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_begin_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer,
								VkSemaphore semaphore);
int vka_end_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_submit_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer,
	VkQueue queue, VkSemaphore *wait_semaphore, VkSemaphore *signal_semaphore);
int vka_wait_for_fence(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);

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
void vka_print_vulkan(FILE *file, vka_vulkan_t *vulkan);
void vka_print_pipeline(FILE *file, vka_pipeline_t *pipeline);
#endif

#endif
