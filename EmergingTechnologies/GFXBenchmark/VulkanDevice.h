#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <SDL.h>
#include "VulkanImage.h"
#include "VulkanSwapchain.h"

struct s_queue_family_indices {
	int graphics_family = -1;
	int present_family = -1;
	int compute_family = -1;
	int transfer_family = -1;

	bool is_complete() const {
		return graphics_family >= 0 && present_family >= 0 && compute_family >= 0 && transfer_family >= 0;
	}
};

class vulkan_device
{
private:
	std::string app_name_;
	SDL_Window* window_;
	VkResult initalize_instance();
	VkResult create_surface(SDL_Window* window);
	VkResult select_physical_device();
	VkResult setup_logical_device();
	VkResult prepare_swapchain();
	VkCommandBuffer begin_single_time_commands(VkCommandPool pool) const;
	void end_single_time_commands(VkQueue queue, VkCommandPool pool, VkCommandBuffer buffer) const;
public:
	vulkan_device(SDL_Window* window, std::string name);
	~vulkan_device() { destroy(); }
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;
	vulkan_swapchain swapchain;
	vulkan_image depthTexture;
	s_queue_family_indices queue_family_indices;
	void initialize(SDL_Window* window);
	void destroy();
	VkResult prepare_depth_resources(const VkQueue& queue, const VkCommandPool& commandPool);
	VkResult prepare_framebuffers(const VkRenderPass& renderpass);
	uint32_t get_device_memory_type(uint32_t filter, VkMemoryPropertyFlags flags) const;
	void create_buffer(const VkDeviceSize size, const VkBufferUsageFlags flags, VkBuffer & buffer) const;
	void create_memory(const VkMemoryPropertyFlags flags, const VkBuffer &buffer, VkDeviceMemory & memory) const;
	void map_memory(void* data, VkDeviceMemory & memory, VkDeviceSize size, VkDeviceSize offset);
	void create_buffer_and_memory(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags flags, VkBuffer & buffer, VkDeviceMemory & memory) const;
	void copy_buffer(VkQueue queue, VkCommandPool pool, VkBuffer dest, VkBuffer src, VkDeviceSize size) const;
	void create_image(uint32_t width,
		uint32_t height,
		uint32_t depth,
		VkImageType type,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags flags,
		VkImage& image,
		VkDeviceMemory& memory);
	void create_image_view(const VkImage& image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect, VkImageView& view) const;
	void transition_image_layout(VkQueue queue, VkCommandPool pool, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageLayout old, VkImageLayout newl);
	void copy_image(VkQueue queue, VkCommandPool pool, VkImage dest, VkImage src, uint32_t width, uint32_t height);
	static bool is_device_vulkan_compatible(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
};
