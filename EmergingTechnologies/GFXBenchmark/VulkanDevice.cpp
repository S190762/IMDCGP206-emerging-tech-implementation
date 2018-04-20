#include "VulkanDevice.h"
#include "VulkanUtil.h"
#include "VulkanImage.h"
#include <set>
#include <iostream>
#include <cassert>
#include <SDL_Vulkan.h>
#include <array>

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation",
	//"VK_LAYER_LUNARG_api_dump"
};

VkResult vulkan_device::initalize_instance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = app_name_.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = app_name_.c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> extensions = vulkan_util::get_instance_required_extensions(window_);

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	assert(vulkan_util::check_validation_layer_support(VALIDATION_LAYERS));
	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

	return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
}

VkResult vulkan_device::create_surface(SDL_Window* window)
{
	SDL_bool result = SDL_Vulkan_CreateSurface(window, instance, &surface);
	if(result != SDL_TRUE)
	{
		throw std::runtime_error("Failed to create a window surface! Vulkan might not be supported by your GPU");
	}
	return VK_SUCCESS;
}

VkResult vulkan_device::select_physical_device()
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0) {
		throw std::runtime_error("Failed to find a GPU that supports Vulkan");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	for (auto pd : physicalDevices) {
		if (is_device_vulkan_compatible(pd, surface)) {
			physical_device = pd;
			break;
		}
	}

	return physical_device != nullptr ? VK_SUCCESS : VK_ERROR_DEVICE_LOST;
}

VkResult vulkan_device::setup_logical_device()
{
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	queue_family_indices = vulkan_util::find_queue_family_indices(physical_device, surface);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { queue_family_indices.graphics_family, queue_family_indices.present_family, queue_family_indices.compute_family };
	for (auto familyIndex : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = familyIndex;
		queueCreateInfo.queueCount = 1; 
		float queuePriority = 1.0f; 
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Append to queue create infos
		queueCreateInfos.push_back(queueCreateInfo);
	}

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	// Grab logical device extensions
	std::vector<const char*> enabledExtensions = vulkan_util::get_device_required_extensions(physical_device);
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

	// Grab validation layers
	assert(vulkan_util::check_validation_layer_support(VALIDATION_LAYERS));
	deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

	// Create the logical device
	VkResult result = vkCreateDevice(physical_device, &deviceCreateInfo, nullptr, &device);

	return result;
}

VkResult vulkan_device::prepare_swapchain()
{
	vulkan_swapchain::swapchain_support swapchainSupport = vulkan_swapchain::query_swapchain_support(physical_device, surface);

	VkSurfaceFormatKHR surfaceFormat = vulkan_swapchain::select_desired_swapchain_surface_format(swapchainSupport.surface_formats);
	VkPresentModeKHR presentMode = vulkan_swapchain::select_desired_swapchain_present_mode(swapchainSupport.present_modes);
	VkExtent2D extent = vulkan_swapchain::select_desired_swapchain_extent(swapchainSupport.capabilities);

	uint32_t minImageCount = swapchainSupport.capabilities.minImageCount + 1;

	if (swapchainSupport.capabilities.maxImageCount > 0 &&
		minImageCount > swapchainSupport.capabilities.maxImageCount) {
		minImageCount = swapchainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.minImageCount = minImageCount;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	assert(queue_family_indices.is_complete());
	if (queue_family_indices.present_family == queue_family_indices.graphics_family) {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr; 
	}
	else {
		const uint32_t indices[2] = {
			static_cast<uint32_t>(queue_family_indices.graphics_family),
			static_cast<uint32_t>(queue_family_indices.present_family),
		};
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = indices;
	}

	swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.clipped = VK_TRUE;

	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.swapchain_);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain!");
	}

	uint32_t imageCount;
	vkGetSwapchainImagesKHR(device, swapchain.swapchain_, &imageCount, nullptr);
	swapchain.images_.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain.swapchain_, &imageCount, swapchain.images_.data());

	swapchain.image_format_ = surfaceFormat.format;
	swapchain.extent_ = extent;
	swapchain.aspect_ratio_ = (float)extent.width / (float)extent.height;

	return result;
}

VkCommandBuffer vulkan_device::begin_single_time_commands(VkCommandPool pool) const
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void vulkan_device::end_single_time_commands(VkQueue queue, VkCommandPool pool, VkCommandBuffer buffer) const
{
	vkEndCommandBuffer(buffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, pool, 1, &buffer);
}

vulkan_device::vulkan_device(SDL_Window* window, std::string name) : app_name_(name), window_(window)
{
	initialize(window);
}

void vulkan_device::initialize(SDL_Window* window)
{
	VkResult result = initalize_instance();
	assert(result == VK_SUCCESS);

	result = create_surface(window);
	assert(result == VK_SUCCESS);

	result = select_physical_device();
	assert(result == VK_SUCCESS);

	result = setup_logical_device();
	assert(result == VK_SUCCESS);

	result = prepare_swapchain();
	assert(result == VK_SUCCESS);
}

void vulkan_device::destroy()
{
	vkDestroyImageView(device, depthTexture.view, nullptr);
	vkDestroyImage(device, depthTexture.image, nullptr);
	vkFreeMemory(device, depthTexture.memory, nullptr);

	vkDestroySwapchainKHR(device, swapchain.swapchain_, nullptr);

	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);
}

VkResult vulkan_device::prepare_depth_resources(const VkQueue& queue, const VkCommandPool& commandPool)
{
	VkFormat depthFormat = vulkan_image::find_depth_format(physical_device);

	create_image(
		swapchain.extent_.width,
		swapchain.extent_.height,
		1,
		VK_IMAGE_TYPE_2D,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthTexture.image,
		depthTexture.memory
	);
	create_image_view(
		depthTexture.image,
		VK_IMAGE_VIEW_TYPE_2D,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		depthTexture.view
	);

	transition_image_layout(
		queue,
		commandPool,
		depthTexture.image,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);

	return VK_SUCCESS;
}

VkResult vulkan_device::prepare_framebuffers(const VkRenderPass& renderpass)
{
	VkResult result = VK_SUCCESS;

	swapchain.image_views_.resize(swapchain.images_.size());
	for (auto i = 0; i < swapchain.image_views_.size(); ++i) {
		create_image_view(
			swapchain.images_[i],
			VK_IMAGE_VIEW_TYPE_2D,
			swapchain.image_format_,
			VK_IMAGE_ASPECT_COLOR_BIT,
			swapchain.image_views_[i]
		);
	}

	swapchain.framebuffers_.resize(swapchain.image_views_.size());

	// Attach image views to framebuffers
	for (int i = 0; i < swapchain.image_views_.size(); ++i) {
		std::array<VkImageView, 1> imageViews = { swapchain.image_views_[i] };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderpass;
		framebufferCreateInfo.attachmentCount = imageViews.size();
		framebufferCreateInfo.pAttachments = imageViews.data();
		framebufferCreateInfo.width = swapchain.extent_.width;
		framebufferCreateInfo.height = swapchain.extent_.height;
		framebufferCreateInfo.layers = 1;

		result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapchain.framebuffers_[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
			return result;
		}
	}

	return result;
}

uint32_t vulkan_device::get_device_memory_type(uint32_t filter, VkMemoryPropertyFlags flags) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

	for (auto i = 0; i < memProperties.memoryTypeCount; ++i) {
		// Loop through each memory type and find a match
		if (filter & (1 << i)) {
			if (memProperties.memoryTypes[i].propertyFlags & flags) {
				return i;
			}
		}
	}

	throw std::runtime_error("Failed to find a suitable memory type");
}

void vulkan_device::create_buffer(const VkDeviceSize size, const VkBufferUsageFlags flags, VkBuffer& buffer) const
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = flags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vulkan_util::check_vulkan_result(
		vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer),
		"Failed to create buffer"
	);
}

void vulkan_device::create_memory(const VkMemoryPropertyFlags flags, const VkBuffer& buffer,
	VkDeviceMemory& memory) const
{
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex =
		get_device_memory_type(memoryRequirements.memoryTypeBits,
			flags);

	VkResult result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory for buffer");
	}
}

void vulkan_device::map_memory(void* data, VkDeviceMemory& memory, VkDeviceSize size, VkDeviceSize offset)
{
	void* temp;
	vkMapMemory(device, memory, offset, size, 0, &temp);
	memcpy(temp, data, size);
	vkUnmapMemory(device, memory);
}

void vulkan_device::create_buffer_and_memory(const VkDeviceSize size, const VkBufferUsageFlags usage,
	const VkMemoryPropertyFlags flags, VkBuffer& buffer, VkDeviceMemory& memory) const
{
	create_buffer(size, usage, buffer);
	create_memory(flags, buffer, memory);
	VkDeviceSize memoryOffset = 0;
	vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

void vulkan_device::copy_buffer(VkQueue queue, VkCommandPool pool, VkBuffer dest, VkBuffer src, VkDeviceSize size) const
{
	VkCommandBuffer copyCommandBuffer = begin_single_time_commands(pool);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(copyCommandBuffer, src, dest, 1, &copyRegion);

	end_single_time_commands(queue, pool, copyCommandBuffer);
}

void vulkan_device::create_image(uint32_t width,
	uint32_t height,
	uint32_t depth,
	VkImageType type,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags flags,
	VkImage& image,
	VkDeviceMemory& memory)
{
	VkImageCreateInfo imageInfo = vulkan_util::make_image_create_info(
		width,
		height,
		depth,
		type,
		format,
		tiling,
		usage
	);

	vulkan_util::check_vulkan_result(
		vkCreateImage(device, &imageInfo, nullptr, &image),
		"Failed to create image"
	);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = get_device_memory_type(
		memRequirements.memoryTypeBits,
		flags
	);

	vulkan_util::check_vulkan_result(
		vkAllocateMemory(device, &memoryAllocInfo, nullptr, &memory),
		"Failed to allocate memory for image"
	);

	vulkan_util::check_vulkan_result(
		vkBindImageMemory(device, image, memory, 0),
		"Failed to bind image memory"
	);
}

void vulkan_device::create_image_view(const VkImage& image, VkImageViewType type, VkFormat format,
	VkImageAspectFlags aspect, VkImageView& view) const
{
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.viewType = type;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = aspect; 
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1; 

	VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &view);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkImageView");
	}
}

void vulkan_device::transition_image_layout(VkQueue queue, VkCommandPool pool, VkImage image, VkFormat format,
	VkImageAspectFlags aspect, VkImageLayout old, VkImageLayout newl)
{

	VkCommandBuffer commandBuffer = begin_single_time_commands(pool);

	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = old;
	imageBarrier.newLayout = newl;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // (this isn't the default value so we must set it)
	imageBarrier.image = image;
	imageBarrier.subresourceRange.aspectMask = aspect;
	if (newl == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		if (vulkan_image::depth_format_has_stencil(format)) {
			imageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	switch (old)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		imageBarrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	}
	switch (newl)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		imageBarrier.dstAccessMask = imageBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		if (imageBarrier.srcAccessMask == 0)
		{
			imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	}


	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&imageBarrier
	);

	end_single_time_commands(queue, pool, commandBuffer);
}

void vulkan_device::copy_image(VkQueue queue, VkCommandPool pool, VkImage dest, VkImage src, uint32_t width,
	uint32_t height)
{
	VkCommandBuffer commandBuffer = begin_single_time_commands(pool);

	// Subresource is sort of like a buffer for images
	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = { 0, 0 };
	region.dstOffset = { 0, 0 };
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
		commandBuffer,
		src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dest, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	end_single_time_commands(queue, pool, commandBuffer);
}

bool vulkan_device::is_device_vulkan_compatible(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	std::vector<const char*> requiredExtensions = vulkan_util::get_device_required_extensions(device);
	bool hasAllRequiredExtensions = requiredExtensions.size() > 0;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

	s_queue_family_indices queueFamilyIndices = vulkan_util::find_queue_family_indices(device, surface);

	vulkan_swapchain::swapchain_support swapchainSupport = vulkan_swapchain::query_swapchain_support(device, surface);

	return hasAllRequiredExtensions &&
		isDiscreteGPU &&
		swapchainSupport.is_complete() &&
		queueFamilyIndices.is_complete();
}




