#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class vulkan_device;

class vulkan_image
{
public:
	int width;
	int height;
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;
	VkSampler sampler;
	VkDescriptorImageInfo descriptor;
	VkFormat format;

	void create(vulkan_device* device, uint32_t w, uint32_t h, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProps);
	void destroy(vulkan_device* device);
	static VkFormat find_supported_format(const VkPhysicalDevice& physical, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	static VkFormat find_depth_format(const VkPhysicalDevice& physical);
	static bool depth_format_has_stencil(VkFormat format);
};
