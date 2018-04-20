#include "VulkanSwapchain.h"
#include <cassert>
#include <algorithm>

vulkan_swapchain::swapchain_support vulkan_swapchain::query_swapchain_support(const VkPhysicalDevice& physical_device,
	const VkSurfaceKHR& surface)
{
	swapchain_support swapchainInfo;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &swapchainInfo.capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		swapchainInfo.surface_formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, swapchainInfo.surface_formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		swapchainInfo.present_modes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, swapchainInfo.present_modes.data());
	}

	return swapchainInfo;
}

VkSurfaceFormatKHR vulkan_swapchain::select_desired_swapchain_surface_format(
	const std::vector<VkSurfaceFormatKHR> available_formats)
{
	assert(available_formats.empty() == false);

	std::vector<VkSurfaceFormatKHR> preferredFormats = {
		{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
	};

	for (const auto& format : available_formats) {
		for (const auto& preferred : preferredFormats) {
			if (format.format == preferred.format && format.colorSpace == preferred.colorSpace) {
				return format;
			}
		}
	}

	return available_formats[0];
}

VkPresentModeKHR vulkan_swapchain::select_desired_swapchain_present_mode(
	const std::vector<VkPresentModeKHR> available_present_modes)
{

	assert(available_present_modes.empty() == false);

	bool enableTrippleBuffering = true;
	if (enableTrippleBuffering) {
		for (const auto& presentMode : available_present_modes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vulkan_swapchain::select_desired_swapchain_extent(const VkSurfaceCapabilitiesKHR surface_capabilities,
	bool use_current_extent, unsigned desired_width, unsigned desired_height)
{
	if (surface_capabilities.currentExtent.width != 0xFFFFFFFF ||
		surface_capabilities.currentExtent.height != 0xFFFFFFFF) {

		return surface_capabilities.currentExtent;
	}

	VkExtent2D extent;

	uint32_t minWidth = surface_capabilities.minImageExtent.width;
	uint32_t maxWidth = surface_capabilities.maxImageExtent.width;
	uint32_t minHeight = surface_capabilities.minImageExtent.height;
	uint32_t maxHeight = surface_capabilities.maxImageExtent.height;
	extent.width = std::max(minWidth, std::min(maxWidth, static_cast<uint32_t>(desired_width)));
	extent.height = std::max(minHeight, std::min(maxHeight, static_cast<uint32_t>(desired_height)));

	return extent;
}
