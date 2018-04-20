#pragma once

#include <glm/glm.hpp>
#include "Renderer.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

struct graphics_uniform_buffer_object
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class vulkan_renderer : public renderer
{
protected:
	VkResult prepare_render_pass();
	VkResult prepare_graphics_pipeline();
	VkResult prepare_post_processing_pipeline();
	VkResult prepare_semaphores();
	vulkan_device* device_;
	VkQueue present_queue_;

	struct
	{
		VkDescriptorSetLayout descriptor_setlayout;
		VkDescriptorPool descriptor_pool;
		VkDescriptorSet descriptor_sets;
		VkPipelineLayout pipeline_layout;
		VkRenderPass render_pass;
		std::vector<geometry_buffer> geometry_buffers;
		VkBuffer uniform_staging_buffer;
		VkBuffer uniform_buffer;
		VkDeviceMemory uniform_staging_buffer_memory;
		VkDeviceMemory uniform_buffer_memory;
		VkPipeline graphics_pipeline;
		VkCommandPool command_pool;
		std::vector<VkCommandBuffer> command_buffers;
		VkQueue graphics_queue;
	} graphics_;

	VkSemaphore image_available_semaphore_;
	VkSemaphore render_finished_semaphore_;
public:
	vulkan_renderer(SDL_Window* window, scene* scene);
	~vulkan_renderer() override;
	void prepare();
	VkResult prepare_descriptor_pool();
	VkResult prepare_descriptor_layouts();
	VkResult prepare_descriptor_sets();
	VkResult prepare_pipelines();
	VkResult prepare_command_pool();
	VkResult build_command_buffers();
	VkResult prepare_uniforms();
	VkResult prepare_vertex_buffers();
	void prepare_textures();
	void update() override;
	void render() override;
};
