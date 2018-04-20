#include "VulkanUtil.h"
#include <algorithm>
#include <SDL_vulkan.h>
#include "VulkanDevice.h"
#include <fstream>

struct VkPresentInfoKHR vulkan_util::make_present_info_khr(const std::vector<VkSemaphore>& wait_semaphores,
	const std::vector<VkSwapchainKHR>& swapchain, const uint32_t* image_indices)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = wait_semaphores.size();
	presentInfo.pWaitSemaphores = wait_semaphores.data();

	presentInfo.swapchainCount = swapchain.size();
	presentInfo.pSwapchains = swapchain.data();
	presentInfo.pImageIndices = image_indices;

	return presentInfo;
}

s_queue_family_indices vulkan_util::find_queue_family_indices(const VkPhysicalDevice& physical_deivce,
	const VkSurfaceKHR& surface)
{
	s_queue_family_indices queueFamilyIndices;

	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_deivce, &queueFamilyPropertyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_deivce, &queueFamilyPropertyCount, queueFamilyProperties.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilyProperties) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphics_family = i;
		}
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_deivce, i, surface, &presentationSupport);

		if (queueFamily.queueCount > 0 && presentationSupport) {
			queueFamilyIndices.present_family = i;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			queueFamilyIndices.compute_family = i;
		}

		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
			queueFamilyIndices.transfer_family = i;
		}

		if (queueFamilyIndices.is_complete()) {
			break;
		}

		++i;
	}

	return queueFamilyIndices;
}

bool
vulkan_util::check_validation_layer_support(
	const std::vector<const char*>& validation_layers
) {

	unsigned int layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availalbeLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availalbeLayers.data());

	// Find the layer
	for (auto layerName : validation_layers) {
		bool foundLayer = false;
		for (auto layerProperty : availalbeLayers) {
			foundLayer = (strcmp(layerName, layerProperty.layerName) == 0);
			if (foundLayer) {
				break;
			}
		}

		if (!foundLayer) {
			return false;
		}
	}
	return true;
}

std::vector<const char*>
vulkan_util::get_instance_required_extensions(SDL_Window * window
) {
	std::vector<const char*> extensions;

	uint32_t extensionCount = 0;
	const char** sdlExtensions;

	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
	sdlExtensions = new const char*[extensionCount];
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, sdlExtensions);

	for (uint32_t i = 0; i < extensionCount; ++i) {
		extensions.push_back(sdlExtensions[i]);
	}

	return extensions;
}

std::vector<const char*>
vulkan_util::get_device_required_extensions(
	const VkPhysicalDevice& physical_device
) {
	const std::vector<const char*> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, availableExtensions.data());

	for (auto& reqruiedExtension : requiredExtensions) {
		if (std::find_if(availableExtensions.begin(), availableExtensions.end(),
			[&reqruiedExtension](const VkExtensionProperties& prop) {
			return strcmp(prop.extensionName, reqruiedExtension) == 0;
		}) == availableExtensions.end()) {
			return {};
		}
	}

	return requiredExtensions;
}

VkDescriptorPoolSize vulkan_util::make_descriptor_pool_size(VkDescriptorType descriptor_type, uint32_t descriptor_count)
{

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = descriptor_type;
	poolSize.descriptorCount = descriptor_count;

	return poolSize;
}

VkDescriptorPoolCreateInfo vulkan_util::make_descriptor_pool_create_info(uint32_t pool_size_count,
	VkDescriptorPoolSize* pool_sizes, uint32_t max_sets)
{
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = pool_size_count;
	descriptorPoolCreateInfo.pPoolSizes = pool_sizes;
	descriptorPoolCreateInfo.maxSets = max_sets;

	return descriptorPoolCreateInfo;
}

VkDescriptorSetLayoutBinding vulkan_util::make_descriptor_set_layout_binding(uint32_t binding,
	VkDescriptorType descriptor_type, VkShaderStageFlags shader_flags, uint32_t descriptor_count)
{
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptor_type;
	layoutBinding.stageFlags = shader_flags;
	layoutBinding.descriptorCount = descriptor_count;

	return layoutBinding;
}

VkDescriptorSetLayoutCreateInfo vulkan_util::make_descriptor_set_layout_create_info(
	VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};

	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = binding_count;
	layoutInfo.pBindings = bindings;

	return layoutInfo;
}

VkDescriptorSetAllocateInfo vulkan_util::make_descriptor_set_allocate_info(VkDescriptorPool descriptor_pool,
	VkDescriptorSetLayout* descriptor_set_layout, uint32_t descriptorSetCount)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptor_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = descriptor_set_layout;

	return allocInfo;;
}

VkDescriptorImageInfo vulkan_util::make_descriptor_image_info(VkImageLayout layout, vulkan_image& image)
{
	VkDescriptorImageInfo imageInfo;

	imageInfo.imageLayout = layout;
	imageInfo.imageView = image.view;
	imageInfo.sampler = image.sampler;

	image.descriptor = imageInfo;

	return imageInfo;
}

VkDescriptorBufferInfo vulkan_util::make_descriptor_buffer_info(VkBuffer buffer, VkDeviceSize offset,
	VkDeviceSize range)
{
	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = buffer;
	descriptorBufferInfo.offset = offset;
	descriptorBufferInfo.range = range;

	return descriptorBufferInfo;
}

VkWriteDescriptorSet vulkan_util::make_write_descriptor_set(VkDescriptorType type, VkDescriptorSet dst_set,
	uint32_t dst_binding, uint32_t descriptor_count, VkDescriptorBufferInfo* buffer_info,
	VkDescriptorImageInfo* image_info)
{
	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dst_set;
	writeDescriptorSet.dstBinding = dst_binding;
	writeDescriptorSet.dstArrayElement = 0; // descriptor set could be an array
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.descriptorCount = descriptor_count;
	writeDescriptorSet.pBufferInfo = buffer_info;
	writeDescriptorSet.pImageInfo = image_info;

	return writeDescriptorSet;
}

VkVertexInputBindingDescription vulkan_util::make_vertex_input_binding_description(uint32_t binding, uint32_t stride,
	VkVertexInputRate rate)
{
	VkVertexInputBindingDescription bindingDesc;
	bindingDesc.binding = binding;
	bindingDesc.stride = stride;
	bindingDesc.inputRate = rate;

	return bindingDesc;
}

VkVertexInputAttributeDescription vulkan_util::make_vertex_input_attribute_description(uint32_t binding,
	uint32_t location, VkFormat format, uint32_t offset)
{
	VkVertexInputAttributeDescription attributeDesc;

	attributeDesc.binding = binding;
	attributeDesc.location = location;
	attributeDesc.format = format;
	attributeDesc.offset = offset;

	return attributeDesc;
}

VkPipelineVertexInputStateCreateInfo vulkan_util::make_pipeline_vertex_input_state_create_info(
	const std::vector<VkVertexInputBindingDescription>& binding_desc,
	const std::vector<VkVertexInputAttributeDescription>& attrib_desc)
{
	VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo = {};
	vertexInputStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputStageCreateInfo.vertexBindingDescriptionCount = binding_desc.size();
	vertexInputStageCreateInfo.pVertexBindingDescriptions = binding_desc.data();
	vertexInputStageCreateInfo.vertexAttributeDescriptionCount = attrib_desc.size();
	vertexInputStageCreateInfo.pVertexAttributeDescriptions = attrib_desc.data();

	return vertexInputStageCreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo vulkan_util::make_pipeline_input_assembly_state_create_info(
	VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = topology;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE; // If true, we can break up primitives like triangels and lines using a special index 0xFFFF

	return inputAssemblyStateCreateInfo;
}

VkViewport vulkan_util::make_fullscreen_viewport(VkExtent2D extent)
{
	VkViewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	return viewport;
}

VkPipelineViewportStateCreateInfo vulkan_util::make_pipeline_viewport_state_create_info(
	const std::vector<VkViewport>& viewports, const std::vector<VkRect2D>& scissors)
{
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = viewports.size();
	viewportStateCreateInfo.pViewports = viewports.data();
	viewportStateCreateInfo.scissorCount = scissors.size();
	viewportStateCreateInfo.pScissors = scissors.data();

	return viewportStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo vulkan_util::make_pipeline_rasterization_state_create_info(
	VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face)
{
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// If enabled, fragments beyond near and far planes are clamped instead of discarded
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	// If enabled, geometry won't pass through rasterization. This would be useful for transform feedbacks
	// where we don't need to go through the fragment shader
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = polygon_mode; // fill, line, or point
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = cull_mode;
	rasterizationStateCreateInfo.frontFace = front_face;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	return rasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo vulkan_util::make_pipeline_multisample_state_create_info(
	VkSampleCountFlagBits sample_count)
{
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = sample_count;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	return multisampleStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo vulkan_util::make_pipeline_depth_stencil_state_create_info(
	VkBool32 depth_test_enable, VkBool32 depth_write_enable, VkCompareOp compare_op)
{
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = depth_test_enable;
	depthStencilStateCreateInfo.depthCompareOp = compare_op; // 1.0f is farthest, 0.0f is closest
	depthStencilStateCreateInfo.depthWriteEnable = depth_write_enable; // Allowing for transparent objects
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE; // Allowing to keep fragment falling withn a  certain range
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {}; // For stencil test

	return depthStencilStateCreateInfo;
}

VkPipelineColorBlendStateCreateInfo vulkan_util::make_pipeline_color_blend_state_create_info(
	const std::vector<VkPipelineColorBlendAttachmentState>& attachments)
{
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.attachmentCount = attachments.size();
	colorBlendStateCreateInfo.pAttachments = attachments.data();

	return colorBlendStateCreateInfo;
}

VkPipelineDynamicStateCreateInfo vulkan_util::make_pipeline_dynamic_state_create_info(
	const VkDynamicState* dynamic_states, uint32_t dynamic_state_count, VkPipelineDynamicStateCreateFlags flags)
{
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = dynamic_states;
	pipelineDynamicStateCreateInfo.dynamicStateCount = dynamic_state_count;
	return pipelineDynamicStateCreateInfo;
}

VkPipelineLayoutCreateInfo vulkan_util::make_pipeline_layout_create_info(VkDescriptorSetLayout* descriptor_set_layouts,
	uint32_t set_layout_count)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = set_layout_count;
	pipelineLayoutCreateInfo.pSetLayouts = descriptor_set_layouts;
	return pipelineLayoutCreateInfo;
}

VkPipelineShaderStageCreateInfo vulkan_util::make_pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
	const VkShaderModule& shader_module)
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = stage;
	shaderStageCreateInfo.module = shader_module;
	// Specify entry point. It's possible to combine multiple shaders into a single shader module
	shaderStageCreateInfo.pName = "main";

	// This can be used to set values for shader constants. The compiler can perform optimization for these constants vs. if they're created as variables in the shaders.
	shaderStageCreateInfo.pSpecializationInfo = nullptr;

	return shaderStageCreateInfo;
}

VkGraphicsPipelineCreateInfo vulkan_util::make_graphics_pipeline_create_info(
	const std::vector<VkPipelineShaderStageCreateInfo>& shader_create_infos,
	const VkPipelineVertexInputStateCreateInfo* vertex_input_stage,
	const VkPipelineInputAssemblyStateCreateInfo* input_assembly_state,
	const VkPipelineTessellationStateCreateInfo* tessellation_state,
	const VkPipelineViewportStateCreateInfo* viewport_state,
	const VkPipelineRasterizationStateCreateInfo* rasterization_state,
	const VkPipelineColorBlendStateCreateInfo* color_blend_state,
	const VkPipelineMultisampleStateCreateInfo* multisample_state,
	const VkPipelineDepthStencilStateCreateInfo* depth_stencil_state,
	const VkPipelineDynamicStateCreateInfo* dynamic_state, const VkPipelineLayout pipeline_layout,
	const VkRenderPass render_pass, const uint32_t subpass, const VkPipeline base_pipeline_handle,
	const int32_t base_pipeline_index)
{
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = shader_create_infos.size(); // Number of shader stages
	graphicsPipelineCreateInfo.pStages = shader_create_infos.data();
	graphicsPipelineCreateInfo.pVertexInputState = vertex_input_stage;
	graphicsPipelineCreateInfo.pInputAssemblyState = input_assembly_state;
	graphicsPipelineCreateInfo.pTessellationState = tessellation_state;
	graphicsPipelineCreateInfo.pViewportState = viewport_state;
	graphicsPipelineCreateInfo.pRasterizationState = rasterization_state;
	graphicsPipelineCreateInfo.pColorBlendState = color_blend_state;
	graphicsPipelineCreateInfo.pMultisampleState = multisample_state;
	graphicsPipelineCreateInfo.pDepthStencilState = depth_stencil_state;
	graphicsPipelineCreateInfo.pDynamicState = dynamic_state;
	graphicsPipelineCreateInfo.layout = pipeline_layout;
	graphicsPipelineCreateInfo.renderPass = render_pass;
	graphicsPipelineCreateInfo.subpass = 0; // Index to the subpass we'll be using

	graphicsPipelineCreateInfo.basePipelineHandle = base_pipeline_handle;
	graphicsPipelineCreateInfo.basePipelineIndex = base_pipeline_index;

	return graphicsPipelineCreateInfo;
}

VkComputePipelineCreateInfo vulkan_util::make_compute_pipeline_create_info(VkPipelineLayout layout,
	VkPipelineCreateFlags flags)
{
	VkComputePipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.layout = layout;
	createInfo.flags = flags;

	return createInfo;
}

std::vector<unsigned char>
read_binary_file(
	const std::string& file_name
) {
	// Read from the end (ate flag) to determine the file size
	std::ifstream file(file_name, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file");
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<unsigned char> buffer(fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	file.close();

	return buffer;
}

void
load_spir_v(
	const char* file_path,
	std::vector<unsigned char>& out_shader
)
{
	out_shader = read_binary_file(file_path);
}

VkShaderModule vulkan_util::make_shader_module(const VkDevice& device, const std::string& filepath)
{
	VkShaderModule module;

	std::vector<unsigned char> bytecode;
	load_spir_v(filepath.c_str(), bytecode);

	VkResult result = VK_SUCCESS;

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = bytecode.size();
	shaderModuleCreateInfo.pCode = (uint32_t*)bytecode.data();

	result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &module);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module");
	}

	return module;
}

VkImageCreateInfo vulkan_util::make_image_create_info(uint32_t width, uint32_t height, uint32_t depth,
	VkImageType image_type, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = image_type;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = depth;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // For multisampling
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // used only by one queue that supports transfer operations
	imageInfo.flags = 0; // We might look into this for flags that support sparse image (if we need to do voxel 3D texture for volumetric)

	return imageInfo;
}

VkRenderPassBeginInfo vulkan_util::make_render_pass_begin_info(const VkRenderPass& render_pass,
	const VkFramebuffer& framebuffer, const VkOffset2D& offset, const VkExtent2D& extent,
	const std::vector<VkClearValue>& clear_values)
{
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = render_pass;
	renderPassBeginInfo.framebuffer = framebuffer;

	// The area where load and store takes place
	renderPassBeginInfo.renderArea.offset = offset;
	renderPassBeginInfo.renderArea.extent = extent;

	renderPassBeginInfo.clearValueCount = clear_values.size();
	renderPassBeginInfo.pClearValues = clear_values.data();

	return renderPassBeginInfo;
}

void vulkan_util::create_default_image_sampler(const VkDevice& device, VkSampler* sampler)
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.maxAnisotropy = 0;
	vulkan_util::check_vulkan_result(
		vkCreateSampler(device, &samplerCreateInfo, nullptr, sampler),
		"Failed to create texture sampler"
	);
}

VkCommandPoolCreateInfo vulkan_util::make_command_pool_create_info(uint32_t queue_family_index)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = queue_family_index;

	// From Vulkan spec:
	// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT indicates that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
	// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT controls whether command buffers allocated from the pool can be individually reset.If this flag is set, individual command buffers allocated from the pool can be reset either explicitly, by calling vkResetCommandBuffer, or implicitly, by calling vkBeginCommandBuffer on an executable command buffer.If this flag is not set, then vkResetCommandBuffer and vkBeginCommandBuffer(on an executable command buffer) must not be called on the command buffers allocated from the pool, and they can only be reset in bulk by calling vkResetCommandPool.
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	return commandPoolCreateInfo;
}

VkCommandBufferAllocateInfo vulkan_util::make_command_buffer_allocate_info(VkCommandPool command_pool,
	VkCommandBufferLevel level, uint32_t bufferCount)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool;
	// Primary means that can be submitted to a queue, but cannot be called from other command buffers
	allocInfo.level = level;
	allocInfo.commandBufferCount = bufferCount;

	return allocInfo;
}

VkCommandBufferBeginInfo vulkan_util::make_command_buffer_begin_info()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	return beginInfo;
}

VkSubmitInfo vulkan_util::make_submit_info(const std::vector<VkSemaphore>& wait_semaphores,
	const std::vector<VkSemaphore>& signal_semaphores, const std::vector<VkPipelineStageFlags>& wait_stage_flags,
	const VkCommandBuffer& command_buffer)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Semaphore to wait on
	submitInfo.waitSemaphoreCount = wait_semaphores.size();
	submitInfo.pWaitSemaphores = wait_semaphores.data(); // The semaphore to wait on
	submitInfo.pWaitDstStageMask = wait_stage_flags.data(); // At which stage to wait on

														  // The command buffer to submit													
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command_buffer;

	// Semaphore to signal
	submitInfo.signalSemaphoreCount = signal_semaphores.size();
	submitInfo.pSignalSemaphores = signal_semaphores.data();

	return submitInfo;
}

VkSubmitInfo vulkan_util::make_submit_info(const VkCommandBuffer& command_buffer)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// The command buffer to submit													
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command_buffer;

	return submitInfo;
}

VkFenceCreateInfo vulkan_util::make_fence_create_info(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.flags = flags;

	return createInfo;
}
