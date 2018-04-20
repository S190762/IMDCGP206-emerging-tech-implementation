#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include <SDL.h>

class vulkan_util
{
public:
	static struct VkPresentInfoKHR
	make_present_info_khr(
		const std::vector<VkSemaphore>& wait_semaphores,
		const std::vector<VkSwapchainKHR>& swapchain,
		const uint32_t* image_indices
	);
	static s_queue_family_indices
		find_queue_family_indices(
			const VkPhysicalDevice& physical_deivce
			, const VkSurfaceKHR& surface
		);
	static bool
		check_validation_layer_support(
			const std::vector<const char*>& validation_layers
		);
	static std::vector<const char*>
		get_device_required_extensions(
			const VkPhysicalDevice& physical_device
		);
	static std::vector<const char*>
		get_instance_required_extensions(SDL_Window * window
		);
	static VkDescriptorPoolSize
	make_descriptor_pool_size(
		VkDescriptorType descriptor_type,
		uint32_t descriptor_count
	);
	static VkDescriptorPoolCreateInfo
	make_descriptor_pool_create_info(
		uint32_t pool_size_count,
		VkDescriptorPoolSize* pool_sizes,
		uint32_t max_sets = 1
	);

	static VkDescriptorSetLayoutBinding
	make_descriptor_set_layout_binding(
		uint32_t binding,
		VkDescriptorType descriptor_type,
		VkShaderStageFlags shader_flags,
		uint32_t descriptor_count = 1
	);

	static VkDescriptorSetLayoutCreateInfo
	make_descriptor_set_layout_create_info(
		VkDescriptorSetLayoutBinding* bindings,
		uint32_t binding_count = 1
	);

	static VkDescriptorSetAllocateInfo
	make_descriptor_set_allocate_info(
		VkDescriptorPool descriptor_pool,
		VkDescriptorSetLayout* descriptor_set_layout,
		uint32_t descriptorSetCount = 1
	);

	static VkDescriptorImageInfo
	make_descriptor_image_info(
		VkImageLayout layout,
		vulkan_image& image
	);

	static VkDescriptorBufferInfo
	make_descriptor_buffer_info(
		VkBuffer buffer,
		VkDeviceSize offset,
		VkDeviceSize range
	);

	static VkWriteDescriptorSet
	make_write_descriptor_set(
		VkDescriptorType type,
		VkDescriptorSet dst_set,
		uint32_t dst_binding,
		uint32_t descriptor_count,
		VkDescriptorBufferInfo* buffer_info,
		VkDescriptorImageInfo* image_info
	);

	static VkVertexInputBindingDescription
	make_vertex_input_binding_description(
		uint32_t binding,
		uint32_t stride,
		VkVertexInputRate rate
	);

	static VkVertexInputAttributeDescription
	make_vertex_input_attribute_description(
		uint32_t binding,
		uint32_t location,
		VkFormat format,
		uint32_t offset
	);

	static VkPipelineVertexInputStateCreateInfo
	make_pipeline_vertex_input_state_create_info(
		const std::vector<VkVertexInputBindingDescription>& binding_desc,
		const std::vector<VkVertexInputAttributeDescription>& attrib_desc
	);

	static VkPipelineInputAssemblyStateCreateInfo
	make_pipeline_input_assembly_state_create_info(
		VkPrimitiveTopology topology
	);

	static VkViewport
	make_fullscreen_viewport(
		VkExtent2D extent
	);

	static VkPipelineViewportStateCreateInfo
	make_pipeline_viewport_state_create_info(
		const std::vector<VkViewport>& viewports,
		const std::vector<VkRect2D>& scissors
	);

	static VkPipelineRasterizationStateCreateInfo
	make_pipeline_rasterization_state_create_info(
		VkPolygonMode polygon_mode,
		VkCullModeFlags cull_mode,
		VkFrontFace front_face
	);

	static VkPipelineMultisampleStateCreateInfo
	make_pipeline_multisample_state_create_info(
		VkSampleCountFlagBits sample_count
	);

	static VkPipelineDepthStencilStateCreateInfo
	make_pipeline_depth_stencil_state_create_info(
		VkBool32 depth_test_enable,
		VkBool32 depth_write_enable,
		VkCompareOp compare_op
	);

	static VkPipelineColorBlendStateCreateInfo
	make_pipeline_color_blend_state_create_info(
		const std::vector<VkPipelineColorBlendAttachmentState>& attachments
	);

	static VkPipelineDynamicStateCreateInfo
	make_pipeline_dynamic_state_create_info(
		const VkDynamicState* dynamic_states,
		uint32_t dynamic_state_count,
		VkPipelineDynamicStateCreateFlags flags);

	static VkPipelineLayoutCreateInfo
	make_pipeline_layout_create_info(
		VkDescriptorSetLayout* descriptor_set_layouts,
		uint32_t set_layout_count = 1
	);

	static VkPipelineShaderStageCreateInfo
	make_pipeline_shader_stage_create_info(
		VkShaderStageFlagBits stage,
		const VkShaderModule& shader_module
	);

	static VkGraphicsPipelineCreateInfo
	make_graphics_pipeline_create_info(
		const std::vector<VkPipelineShaderStageCreateInfo>& shader_create_infos,
		const VkPipelineVertexInputStateCreateInfo* vertex_input_stage,
		const VkPipelineInputAssemblyStateCreateInfo* input_assembly_state,
		const VkPipelineTessellationStateCreateInfo* tessellation_state,
		const VkPipelineViewportStateCreateInfo* viewport_state,
		const VkPipelineRasterizationStateCreateInfo* rasterization_state,
		const VkPipelineColorBlendStateCreateInfo* color_blend_state,
		const VkPipelineMultisampleStateCreateInfo* multisample_state,
		const VkPipelineDepthStencilStateCreateInfo* depth_stencil_state,
		const VkPipelineDynamicStateCreateInfo* dynamic_state,
		const VkPipelineLayout pipeline_layout,
		const VkRenderPass render_pass,
		const uint32_t subpass,
		const VkPipeline base_pipeline_handle,
		const int32_t base_pipeline_index
	);

	static VkComputePipelineCreateInfo
	make_compute_pipeline_create_info(
		VkPipelineLayout layout,
		VkPipelineCreateFlags flags
	);

	static VkShaderModule
	make_shader_module(
		const VkDevice& device,
		const std::string& filepath
	);
	static VkImageCreateInfo
	make_image_create_info(
		uint32_t width,
		uint32_t height,
		uint32_t depth,
		VkImageType image_type,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage
	);
	static VkRenderPassBeginInfo
	make_render_pass_begin_info(
		const VkRenderPass& render_pass,
		const VkFramebuffer& framebuffer,
		const VkOffset2D& offset,
		const VkExtent2D& extent,
		const std::vector<VkClearValue>& clear_values // for each attachment
	);
	static void
	create_default_image_sampler(
		const VkDevice& device,
		VkSampler* sampler
	);
	static VkCommandPoolCreateInfo
	make_command_pool_create_info(
		uint32_t queue_family_index
	);

	static VkCommandBufferAllocateInfo
	make_command_buffer_allocate_info(
		VkCommandPool command_pool,
		VkCommandBufferLevel level,
		uint32_t bufferCount
	);

	static VkCommandBufferBeginInfo
	make_command_buffer_begin_info();

	static VkSubmitInfo
	make_submit_info(
		const std::vector<VkSemaphore>& wait_semaphores,
		const std::vector<VkSemaphore>& signal_semaphores,
		const std::vector<VkPipelineStageFlags>& wait_stage_flags,
		const VkCommandBuffer& command_buffer
	);

	static VkSubmitInfo
	make_submit_info(
		const VkCommandBuffer& command_buffer
	);

	static VkFenceCreateInfo
	make_fence_create_info(
		VkFenceCreateFlags flags
	);

	static void check_vulkan_result(
		VkResult result,
		std::string message
	)
	{
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(message);
		}
	}

	static void CreateTextureImage(
	)
	{
	}
};


