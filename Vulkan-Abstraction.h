#ifndef VULKAN_ABSTRACTION_H
#define VULKAN_ABSTRACTION_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NM-Config/Config.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <Volk/volk.h>

#ifdef VKA_NUKLEAR
#define VKA_NUKLEAR_MAX_VERTEX_BUFFER 512 * 1024
#define VKA_NUKLEAR_MAX_INDEX_BUFFER 128 * 1024
#define NK_ASSERT(expr)
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_IO
#include <Nuklear/nuklear.h>
#endif

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

#define VKA_BUFFER_USAGE_INDEX VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
#define VKA_BUFFER_USAGE_VERTEX VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
#define VKA_BUFFER_USAGE_UNIFORM VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
#define VKA_BUFFER_USAGE_STORAGE VK_BUFFER_USAGE_TRANSFER_DST_BIT | \
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
#define VKA_BUFFER_USAGE_STAGING VK_BUFFER_USAGE_TRANSFER_SRC_BIT

#define VKA_MEMORY_HOST VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
#define VKA_MEMORY_DEVICE VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

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
	VkDescriptorPool pool;

	/*---------------*
	 * Configuration *
	 *---------------*/
	uint32_t max_sets;
	uint32_t num_uniform_buffers;
	uint32_t num_storage_buffers;
	uint32_t num_storage_images;
	uint32_t num_image_samplers;
} vka_descriptor_pool_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;
	void **data;
	vka_descriptor_pool_t *pool;

	/*---------------*
	 * Configuration *
	 *---------------*/
	uint32_t binding;
	uint32_t count;
	VkDescriptorType type;
	VkShaderStageFlags stage_flags;
} vka_descriptor_set_t;

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

	// These are created and updated automatically - don't touch.
	VkDescriptorSetLayout *descriptor_layout_tracker;
	VkDescriptorSet *descriptor_set_tracker;

	/*---------------*
	 * Configuration *
	 *---------------*/
	int is_compute_pipeline;

	uint32_t num_descriptor_sets;
	vka_descriptor_set_t **descriptor_sets;

	// Vertex input:
	uint32_t num_vertex_bindings;
	uint32_t strides[VKA_MAX_VERTEX_ATTRIBUTES];
	VkVertexInputRate input_rates[VKA_MAX_VERTEX_ATTRIBUTES]; // Default ...INPUT_RATE_VERTEX.
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
	VkMemoryPropertyFlags properties[2];	// First choice, second choice.
	VkDeviceSize map_offset;
	VkDeviceSize map_size;			// If 0, uses VK_WHOLE_SIZE.
} vka_allocation_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkBuffer buffer;
	vka_allocation_t *allocation;
	void *data; // Intended for scene uniform buffer updates. Is NOT managed by buffer struct.

	/*---------------*
	 * Configuration *
	 *---------------*/
	VkDeviceSize size;
	VkDeviceSize offset;		// Offset inside allocation.
	VkBufferUsageFlags usage;
	VkIndexType index_type;		// For index buffer.
} vka_buffer_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkImage image;
	VkImageView image_view;

	/*---------------*
	 * Configuration *
	 *---------------*/
	uint8_t is_swapchain_image;	// Prevents invalid creation/destruction of VkImage.
	VkFormat format;
	VkImageAspectFlags aspect_mask;
	uint32_t mip_levels;
} vka_image_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	VkSampler sampler;

	/*---------------*
	 * Configuration *
	 *---------------*/
	VkBool32 anisotropy_enable;
	float max_anisotropy;
	VkBorderColor border_colour;
} vka_sampler_t;

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
	vka_image_t *swapchain_images;

	uint8_t recreate_swapchain;
	uint8_t recreate_pipelines;
	uint8_t current_frame;
	uint32_t current_swapchain_index;

	char error[NM_MAX_ERROR_LENGTH];
	#ifdef VKA_DEBUG
	VkDebugUtilsMessengerEXT debug_messenger;
	#endif

	#ifdef VKA_NUKLEAR
	struct nk_context nuklear_context;
	struct nk_buffer nuklear_commands;
	struct nk_draw_null_texture nuklear_null_texture;
	vka_allocation_t nuklear_allocation;
	vka_buffer_t nuklear_buffer_index;
	vka_buffer_t nuklear_buffer_vertex;
	#endif

	/*---------------*
	 * Configuration *
	 *---------------*/
	uint8_t window_resizable;
	int minimum_window_width;
	int minimum_window_height;

	// Feature requirements (after device creation, these represent enabled features):
	VkPhysicalDeviceFeatures enabled_features;
	VkPhysicalDeviceVulkan11Features enabled_features_11;
	VkPhysicalDeviceVulkan12Features enabled_features_12;
	VkPhysicalDeviceVulkan13Features enabled_features_13;
} vka_vulkan_t;

/**************************
 * Information containers *
 **************************/

typedef struct
{
	vka_image_t *colour_image;
	VkClearValue colour_clear_value;
	VkAttachmentLoadOp colour_load_op;
	VkAttachmentStoreOp colour_store_op;

	vka_image_t *depth_image;
	VkClearValue depth_clear_value;
	VkAttachmentLoadOp depth_load_op;
	VkAttachmentStoreOp depth_store_op;

	VkRect2D render_area;
	uint32_t render_target_height;
	VkRect2D scissor_area;
} vka_render_info_t;

typedef struct
{
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
		VkBool32 *sampler_anisotropy);

// Pipelines and shaders:
int vka_create_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline);
void vka_destroy_pipeline(vka_vulkan_t *vulkan, vka_pipeline_t *pipeline);
void vka_bind_pipeline(vka_command_buffer_t *command_buffer, vka_pipeline_t *pipeline);
int vka_create_shader(vka_vulkan_t *vulkan, vka_shader_t *shader);
int vka_create_shader_from_array(vka_vulkan_t *vulkan, vka_shader_t *shader,
				size_t code_size, uint32_t *shader_code);
void vka_destroy_shader(vka_vulkan_t *vulkan, vka_shader_t *shader);

// Command buffers and fences:
int vka_create_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
void vka_destroy_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_begin_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_end_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_submit_command_buffer(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);
int vka_wait_for_fence(vka_vulkan_t *vulkan, vka_command_buffer_t *command_buffer);

// Descriptors:
int vka_create_descriptor_pool(vka_vulkan_t *vulkan, vka_descriptor_pool_t *descriptor_pool);
void vka_destroy_descriptor_pool(vka_vulkan_t *vulkan, vka_descriptor_pool_t *descriptor_pool);
int vka_create_descriptor_set_layout(vka_vulkan_t *vulkan, vka_descriptor_set_t *descriptor_set);
void vka_destroy_descriptor_set(vka_vulkan_t *vulkan, vka_descriptor_set_t *descriptor_set);
int vka_allocate_descriptor_set(vka_vulkan_t *vulkan, vka_descriptor_set_t *descriptor_set);
int vka_update_descriptor_set(vka_vulkan_t *vulkan, vka_descriptor_set_t *descriptor_set);

// Samplers, images and image views:
int vka_create_sampler(vka_vulkan_t *vulkan, vka_sampler_t *sampler);
void vka_destroy_sampler(vka_vulkan_t *vulkan, vka_sampler_t *sampler);
int vka_create_image(vka_vulkan_t *vulkan, vka_image_t *image);
void vka_destroy_image(vka_vulkan_t *vulkan, vka_image_t *image);
void vka_transition_image(vka_command_buffer_t *command_buffer, vka_image_t *image,
						vka_image_info_t *image_info);

// Buffers:
int vka_create_buffer(vka_vulkan_t *vulkan, vka_buffer_t *buffer);
void vka_destroy_buffer(vka_vulkan_t *vulkan, vka_buffer_t *buffer);
int vka_get_buffer_requirements(vka_vulkan_t *vulkan, vka_buffer_t *buffer);
int vka_bind_buffer_memory(vka_vulkan_t *vulkan, vka_buffer_t *buffer);
void vka_copy_buffer(vka_command_buffer_t *command_buffer,
	vka_buffer_t *source, vka_buffer_t *destination);
void vka_update_buffer(vka_command_buffer_t *command_buffer, vka_buffer_t *buffer);

// Rendering:
void vka_begin_rendering(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_end_rendering(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_set_viewport(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_set_scissor(vka_command_buffer_t *command_buffer, vka_render_info_t *render_info);
void vka_bind_vertex_buffers(vka_command_buffer_t *command_buffer, vka_buffer_t *index_buffer,
				uint32_t num_vertex_buffers, vka_buffer_t vertex_buffers[]);
void vka_bind_descriptor_sets(vka_command_buffer_t *command_buffer, vka_pipeline_t *pipeline);
void vka_draw(vka_command_buffer_t *command_buffer, uint32_t num_vertices, int32_t vertex_offset);
void vka_draw_indexed(vka_command_buffer_t *command_buffer, uint32_t num_indices,
				uint32_t index_offset, int32_t vertex_offset);
int vka_present_image(vka_vulkan_t *vulkan);

// Memory:
int vka_create_allocation(vka_vulkan_t *vulkan, vka_allocation_t *allocation);
void vka_destroy_allocation(vka_vulkan_t *vulkan, vka_allocation_t *allocation);
int vka_map_memory(vka_vulkan_t *vulkan, vka_allocation_t *allocation);
void vka_unmap_memory(vka_vulkan_t *vulkan, vka_allocation_t *allocation);

// Misc:
void vka_device_wait_idle(vka_vulkan_t *vulkan);
void vka_next_frame(vka_vulkan_t *vulkan);
int vka_get_next_swapchain_image(vka_vulkan_t *vulkan);

#ifdef VKA_NUKLEAR
typedef struct
{
	float position[2];
	uint8_t colour[4];
	float uv[2];
} vka_nuklear_vertex_t;

int vka_nuklear_setup(vka_vulkan_t *vulkan);
void vka_nuklear_shutdown(vka_vulkan_t *vulkan);
void vka_nuklear_draw(vka_vulkan_t *vulkan);
void vka_nuklear_process_event(vka_vulkan_t *vulkan, SDL_Event *event);
void vka_nuklear_process_grab(vka_vulkan_t *vulkan);
void vka_nuklear_clipboard_copy(nk_handle usr, const char *text, int len);
void vka_nuklear_clipboard_paste(nk_handle usr, struct nk_text_edit *edit);
#endif

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
void vka_print_command_buffer(FILE *file, vka_command_buffer_t *command_buffer);
void vka_print_image(FILE *file, vka_image_t *image);
void vka_print_shader(FILE *file, vka_shader_t *shader);
void vka_print_pipeline(FILE *file, vka_pipeline_t *pipeline);
void vka_print_descriptor_pool(FILE *file, vka_descriptor_pool_t *descriptor_pool);
void vka_print_allocation(FILE *file, vka_allocation_t *allocation);
void vka_print_buffer(FILE *file, vka_buffer_t *buffer);
void vka_print_sampler(FILE *file, vka_sampler_t *sampler);
#endif

#endif
