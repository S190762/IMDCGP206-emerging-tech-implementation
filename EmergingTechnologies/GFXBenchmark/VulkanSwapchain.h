#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class vulkan_swapchain
{
public:
	VkSwapchainKHR swapchain_;
	std::vector<VkImage> images_;
	VkFormat image_format_;
	VkExtent2D extent_;
	float aspect_ratio_;
	std::vector<VkImageView> image_views_;
	std::vector<VkFramebuffer> framebuffers_;

	struct swapchain_support
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> surface_formats;
		std::vector<VkPresentModeKHR> present_modes;

		bool is_complete() const
		{
			return !surface_formats.empty() && !present_modes.empty();
		}
	};

	static swapchain_support query_swapchain_support(
		const VkPhysicalDevice& physical_device
		, const VkSurfaceKHR& surface
	);

	static VkSurfaceFormatKHR select_desired_swapchain_surface_format(
		const std::vector<VkSurfaceFormatKHR> available_formats
	);

	static VkPresentModeKHR select_desired_swapchain_present_mode(
		const std::vector<VkPresentModeKHR> available_present_modes
	);

	static VkExtent2D select_desired_swapchain_extent(
		const VkSurfaceCapabilitiesKHR surface_capabilities
		, bool use_current_extent = true
		, unsigned int desired_width = 0
		, unsigned int desired_height = 0
	);
};
