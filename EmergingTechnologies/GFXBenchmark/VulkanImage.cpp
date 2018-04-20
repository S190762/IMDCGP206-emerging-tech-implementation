#include "VulkanImage.h"
#include "VulkanDevice.h"

void vulkan_image::create(vulkan_device* device, uint32_t w, uint32_t h, VkImageTiling tiling,
	VkImageUsageFlags usage, VkMemoryPropertyFlags memProps)
{
	VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

	width = w;
	height = h;

	device->create_image(
		width,
		height,
		1, // only a 2D depth image
		VK_IMAGE_TYPE_2D,
		imageFormat,
		tiling,
		// Image is sampled in fragment shader and used as storage for compute output
		usage,
		memProps,
		image,
		memory
	);
}

void vulkan_image::destroy(vulkan_device* device)
{
	vkDestroySampler(device->device, sampler, nullptr);
	vkDestroyImageView(device->device, view, nullptr);
	vkDestroyImage(device->device, image, nullptr);
	vkFreeMemory(device->device, memory, nullptr);
}

VkFormat vulkan_image::find_supported_format(const VkPhysicalDevice& physical, const std::vector<VkFormat>& candidates,
	VkImageTiling tiling, VkFormatFeatureFlags features)
{

	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find a supported format");
}

VkFormat vulkan_image::find_depth_format(const VkPhysicalDevice& physical)
{
	return find_supported_format(
		physical,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool vulkan_image::depth_format_has_stencil(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
