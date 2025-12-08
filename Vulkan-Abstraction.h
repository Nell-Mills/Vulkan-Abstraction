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

/* Note - pointers to Vk structs are managed outside their containers.
 * Exception: swapchain images and image views in vka_vulkan_t. */

/*************************
 * Functional containers *
 *************************/

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkCommandBuffer buffer;
	VkFence fence;

	/*---------------*
	 * Configuration *
	 *---------------*/
	uint8_t fence_signaled;

	uint8_t use_wait;
	uint8_t use_signal;
	VkPipelineStageFlags wait_dst_stage_mask;
	VkSemaphore *wait_semaphore;
	VkSemaphore *signal_semaphore;

	VkQueue *queue;
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

	/*---------------*
	 * Configuration *
	 *---------------*/
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

	/*---------------*
	 * Configuration *
	 *---------------*/
	int is_compute_pipeline;

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
	float line_width;		// If < 1.f, gets set to 1.f for graphics pipelines.

	// TODO multisampling.
	// TODO depth stencil.

	// Blend:
	VkBool32 blend_enable;		// Default VK_FALSE.
	VkBlendOp colour_blend_op;	// Default VK_BLEND_OP_ADD.
	VkBlendOp alpha_blend_op;
	VkColorComponentFlags colour_write_mask; // If 0, gets set to RGBA for graphics pipelines.

	// Rendering:
	VkFormat colour_attachment_format; // Default VK_FORMAT_UNDEFINED.
	VkFormat depth_attachment_format;
} vka_pipeline_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkDeviceMemory memory;
	void *mapped_data;

	/*---------------*
	 * Configuration *
	 *---------------*/
	VkMemoryRequirements requirements;
	VkMemoryPropertyFlags properties[2]; // First choice, second choice.
	VkOffset map_offset;
	VkDeviceSize map_size;
} vka_allocation_t;

/**************************
 * Information containers *
 **************************/

typedef struct
{
	VkImage *colour_image;
	VkImageView *colour_image_view;
	VkClearValue colour_clear_value;
	VkAttachmentLoadOp colour_load_op;
	VkAttachmentStoreOp colour_store_op;

	VkImageView *depth_image_view;
	VkClearValue depth_clear_value;
	VkAttachmentLoadOp depth_load_op;
	VkAttachmentStoreOp depth_store_op;

	VkRect2D render_area;
	uint32_t render_target_height;
	VkRect2D scissor_area;
} vka_render_info_t;

typedef struct
{
	VkImage *image;
	uint32_t mip_levels;

	VkAccessFlags src_access_mask;
	VkAccessFlags dst_access_mask;
	VkImageLayout old_layout;
	VkImageLayout new_layout;
	VkPipelineStageFlags src_stage_mask;
	VkPipelineStageFlags dst_stage_mask;
} vka_image_info_t;

// Main Vulkan base:
int vka_setup_vulkan(vka_vulkan_t *vulkan);
void vka_shutdown_vulkan(vka_vulkan_t *vulkan);
int vka_create_window(vka_vulkan_t *vulkan);
int vka_create_instance(vka_vulkan_t *vulkan);
int vka_create_surface(vka_vulkan_t *vulkan);
int vka_create_device(vka_vulkan_t *vulkan);
int vka_create_semaphores(vka_vulkan_t *vulkan);
int vka_create_command_pool(vka_vulkan_t *vulkan);
int vka_create_command_buffers(vka_vulkan_t *vulkan);
int vka_create_swapchain(vka_vulkan_t *vulkan);
int vka_score_physical_device(vka_vulkan_t *vulkan, VkPhysicalDevice physical_device,
	uint32_t *graphics_family_index, uint32_t *present_family_index,
	VkDeviceSize *max_memory_allocation_size, VkBool32 *sampler_anisotropy);

// Pipelines and shaders:
int vka_create_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline);
void vka_destroy_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline);
void vka_bind_pipeline(vka_command_buffer_t *command_buffer, vka_pipeline_t *pipeline);
int vka_create_shader(vka_vulkan_t *vulkan, vka_shader_t *shader);
void vka_destroy_shader(vka_vulkan_t *vulkan, vka_shader_t *shader);

// Command buffers and fences:
int vka_create_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
void vka_destroy_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_begin_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_end_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_submit_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_wait_for_fence(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);

// Images and image views:
void vka_transition_image(vka_command_buffer_t *command_buffer, vka_image_info_t *image_info);

// Rendering:
void vka_begin_rendering(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_end_rendering(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_set_viewport(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_set_scissor(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
int vka_present_image(vka_vulkan_t *vulkan);

// Memory:
int vka_create_allocation(vka_vulkan_t *vulkan, vka_allocation_t *allocation);
void vka_destroy_allocation(vka_vulkan_t *vulkan, vka_allocation_t *allocation);
int vka_map_memory(vka_vulkan_t *vulkan, vka_allocation_t *allocation);
void vka_unmap_memory(vka_vulkan_t *vulkan, vka_allocation_t *allocation);

// Utility:
void vka_device_wait_idle(vka_vulkan_t *vulkan);
void vka_next_frame(vka_vulkan_t *vulkan);
int vka_get_next_swapchain_image(vka_vulkan_t *vulkan);

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
