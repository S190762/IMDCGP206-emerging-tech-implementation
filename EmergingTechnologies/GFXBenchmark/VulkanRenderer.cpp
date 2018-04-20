#include "VulkanRenderer.h"
#include <array>
#include "VulkanUtil.h"
#include "SceneUtil.h"
#include "VulkanBuffer.h"
#include <glm/glm.hpp>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>p
#include <iostream>

VkResult vulkan_renderer::prepare_render_pass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = device_->swapchain.image_format_;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; 
														  
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
															
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; 

																   
																   
																   
	VkAttachmentReference colorAttachmentRef = {};
	
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	

	/*VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = vulkan_image::find_depth_format(device_->physical_device);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; 
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

	

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	//subpass.pDepthStencilAttachment = &depthAttachmentRef;

	
	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	

	std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachments.size();
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	VkResult result = vkCreateRenderPass(device_->device, &renderPassCreateInfo, nullptr, &graphics_.render_pass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
		return result;
	}

	return result;
}

VkResult vulkan_renderer::prepare_graphics_pipeline()
{
	VkResult result = VK_SUCCESS;

	
	
	
	VkShaderModule vertShader = vulkan_util::make_shader_module(device_->device, "shaders/vert.spv");
	VkShaderModule fragShader = vulkan_util::make_shader_module(device_->device, "shaders/frag.spv");

	
	
	
	vertex_attribute_info positionAttrib = scene_->meshesData[0]->attrib_info.at(POSITION);
	vertex_attribute_info normalAttrib = scene_->meshesData[0]->attrib_info.at(NORMAL);
	std::vector<VkVertexInputBindingDescription> bindingDesc = {
		vulkan_util::make_vertex_input_binding_description(
			0, 
			sizeof(glm::vec3),
			VK_VERTEX_INPUT_RATE_VERTEX
		),
		vulkan_util::make_vertex_input_binding_description(
			1, 
			sizeof(glm::vec3),
			VK_VERTEX_INPUT_RATE_VERTEX
		),
	};


	
	std::vector<VkVertexInputAttributeDescription> attribDesc = {
		vulkan_util::make_vertex_input_attribute_description(
			0, 
			0, 
			VK_FORMAT_R32G32B32_SFLOAT,
			0 
		),
		vulkan_util::make_vertex_input_attribute_description(
			0, 
			1, 
			VK_FORMAT_R32G32B32_SFLOAT,
			0 
		)
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStageCreateInfo = vulkan_util::make_pipeline_vertex_input_state_create_info(
		bindingDesc,
		attribDesc
	);


	
	
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
		vulkan_util::make_pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	
	


	
	
	
	std::vector<VkViewport> viewports = {
		vulkan_util::make_fullscreen_viewport(device_->swapchain.extent_)
	};

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = device_->swapchain.extent_;

	std::vector<VkRect2D> scissors = {
		scissor
	};

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = vulkan_util::make_pipeline_viewport_state_create_info(viewports, scissors);

	
	
	
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vulkan_util::make_pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	
	

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
		vulkan_util::make_pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

	
	
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = vulkan_util::make_pipeline_depth_stencil_state_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	
	
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = {
		colorBlendAttachmentState
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vulkan_util::make_pipeline_color_blend_state_create_info(colorBlendAttachments);

	
	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState =
		vulkan_util::make_pipeline_dynamic_state_create_info(
			dynamicStateEnables.data(),
			static_cast<uint32_t>(dynamicStateEnables.size()),
			0);

	
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vulkan_util::make_pipeline_layout_create_info(&graphics_.descriptor_setlayout);
	pipelineLayoutCreateInfo.pSetLayouts = &graphics_.descriptor_setlayout;
	vulkan_util::check_vulkan_result(
		vkCreatePipelineLayout(device_->device, &pipelineLayoutCreateInfo, nullptr, &graphics_.pipeline_layout),
		"Failed to create pipeline layout."
	);

	
	

	std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos = {
		vulkan_util::make_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
		vulkan_util::make_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	};

	

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		vulkan_util::make_graphics_pipeline_create_info(
			shaderCreateInfos,
			&vertexInputStageCreateInfo,
			&inputAssemblyStateCreateInfo,
			nullptr,
			&viewportStateCreateInfo,
			&rasterizationStateCreateInfo,
			&colorBlendStateCreateInfo,
			&multisampleStateCreateInfo,
			nullptr,
			nullptr,
			graphics_.pipeline_layout,
			graphics_.render_pass,
			0, 

			   
			   
			VK_NULL_HANDLE,
			-1
		);

	
	vulkan_util::check_vulkan_result(
		vkCreateGraphicsPipelines(
			device_->device,
			VK_NULL_HANDLE, 
			1, 
			&graphicsPipelineCreateInfo,
			nullptr,
			&graphics_.graphics_pipeline 
		),
		"Failed to create graphics pipeline"
	);

	
	vkDestroyShaderModule(device_->device, vertShader, nullptr);
	vkDestroyShaderModule(device_->device, fragShader, nullptr);

	return result;
}

VkResult vulkan_renderer::prepare_semaphores()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(device_->device, &semaphoreCreateInfo, nullptr, &image_available_semaphore_);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create imageAvailable semaphore");
		return result;
	}

	result = vkCreateSemaphore(device_->device, &semaphoreCreateInfo, nullptr, &render_finished_semaphore_);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create renderFinished semaphore");
		return result;
	}

	return result;
}

vulkan_renderer::vulkan_renderer(SDL_Window* window, scene* scene) : renderer(window, scene)
{
	device_ = new vulkan_device(window_, "Vulkan");
	height_ = device_->swapchain.extent_.height;
	width_ = device_->swapchain.extent_.width;

	
	vkGetDeviceQueue(device_->device, device_->queue_family_indices.graphics_family, 0, &graphics_.graphics_queue);

	
	vkGetDeviceQueue(device_->device, device_->queue_family_indices.present_family, 0, &present_queue_);

	VkResult result;
	result = prepare_command_pool();
	assert(result == VK_SUCCESS);
	std::cout << result << std::endl;

	//result = device_->prepare_depth_resources(graphics_.graphics_queue, graphics_.command_pool);
	assert(result == VK_SUCCESS);

//	result = prepare_render_pass();
	assert(result == VK_SUCCESS);

	//result = device_->prepare_framebuffers(graphics_.render_pass);
	assert(result == VK_SUCCESS);

	//result = prepare_semaphores();
	assert(result == VK_SUCCESS);

	prepare();

}

vulkan_renderer::~vulkan_renderer()
{
	
	vkDeviceWaitIdle(device_->device);

	

	vkDestroySemaphore(device_->device, image_available_semaphore_, nullptr);
	vkDestroySemaphore(device_->device, render_finished_semaphore_, nullptr);

	vkFreeCommandBuffers(device_->device, graphics_.command_pool, graphics_.command_buffers.size(), graphics_.command_buffers.data());

	vkDestroyDescriptorPool(device_->device, graphics_.descriptor_pool, nullptr);

	for (geometry_buffer& geomBuffer : graphics_.geometry_buffers) {
		vkFreeMemory(device_->device, geomBuffer.vertex_buffer_memory, nullptr);
		vkDestroyBuffer(device_->device, geomBuffer.vertex_buffer, nullptr);
	}

	vkFreeMemory(device_->device, graphics_.uniform_staging_buffer_memory, nullptr);
	vkDestroyBuffer(device_->device, graphics_.uniform_staging_buffer, nullptr);
	vkFreeMemory(device_->device, graphics_.uniform_buffer_memory, nullptr);
	vkDestroyBuffer(device_->device, graphics_.uniform_buffer, nullptr);

	vkDestroyCommandPool(device_->device, graphics_.command_pool, nullptr);
	for (auto& frameBuffer : device_->swapchain.framebuffers_) {
		vkDestroyFramebuffer(device_->device, frameBuffer, nullptr);
	}

	vkDestroyRenderPass(device_->device, graphics_.render_pass, nullptr);

	vkDestroyDescriptorSetLayout(device_->device, graphics_.descriptor_setlayout, nullptr);


	vkDestroyPipelineLayout(device_->device, graphics_.pipeline_layout, nullptr);
	for (auto& imageView : device_->swapchain.image_views_) {
		vkDestroyImageView(device_->device, imageView, nullptr);
	}
	vkDestroyPipeline(device_->device, graphics_.graphics_pipeline, nullptr);

	delete device_;
}

void vulkan_renderer::prepare()
{
	VkResult result;

	result = prepare_vertex_buffers();
	assert(result == VK_SUCCESS);

	result = prepare_uniforms();
	assert(result == VK_SUCCESS);

	result = prepare_descriptor_pool();
	assert(result == VK_SUCCESS);

	result = prepare_descriptor_layouts();
	assert(result == VK_SUCCESS);

	result = prepare_descriptor_sets();
	assert(result == VK_SUCCESS);

	result = prepare_pipelines();
	assert(result == VK_SUCCESS);

	result = build_command_buffers();
	assert(result == VK_SUCCESS);
}

VkResult vulkan_renderer::prepare_descriptor_pool()
{
	VkDescriptorPoolSize poolSize = vulkan_util::make_descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vulkan_util::make_descriptor_pool_create_info(1, &poolSize, 1);

	vulkan_util::check_vulkan_result(
		vkCreateDescriptorPool(device_->device, &descriptorPoolCreateInfo, nullptr, &graphics_.descriptor_pool),
		"Failed to create descriptor pool"
	);

	return VK_SUCCESS;
}

VkResult vulkan_renderer::prepare_descriptor_layouts()
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		
		vulkan_util::make_descriptor_set_layout_binding(
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT
		),
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
		vulkan_util::make_descriptor_set_layout_create_info(
			setLayoutBindings.data(),
			setLayoutBindings.size()
		);

	VkResult result = vkCreateDescriptorSetLayout(device_->device, &descriptorSetLayoutCreateInfo, nullptr, &graphics_.descriptor_setlayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
		return result;
	}

	return result;
}

VkResult vulkan_renderer::prepare_descriptor_sets()
{
	VkDescriptorSetAllocateInfo allocInfo = vulkan_util::make_descriptor_set_allocate_info(graphics_.descriptor_pool, &graphics_.descriptor_setlayout);

	vulkan_util::check_vulkan_result(
		vkAllocateDescriptorSets(device_->device, &allocInfo, &graphics_.descriptor_sets),
		"Failed to allocate descriptor set"
	);

	VkDescriptorBufferInfo bufferInfo = vulkan_util::make_descriptor_buffer_info(graphics_.uniform_buffer, 0, sizeof(graphics_uniform_buffer_object));

	
	VkWriteDescriptorSet descriptorWrite = vulkan_util::make_write_descriptor_set(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		graphics_.descriptor_sets,
		0,
		1,
		&bufferInfo,
		nullptr
	);

	vkUpdateDescriptorSets(device_->device, 1, &descriptorWrite, 0, nullptr);

	return VK_SUCCESS;
}

VkResult vulkan_renderer::prepare_pipelines()
{
	return prepare_graphics_pipeline();
}

VkResult vulkan_renderer::prepare_command_pool()
{
	VkResult result = VK_SUCCESS;

	
	VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo = vulkan_util::make_command_pool_create_info(device_->queue_family_indices.graphics_family);

	result = vkCreateCommandPool(device_->device, &graphicsCommandPoolCreateInfo, nullptr, &graphics_.command_pool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool.");
		return result;
	}

	return result;
}

VkResult vulkan_renderer::build_command_buffers()
{
	graphics_.command_buffers.resize(device_->swapchain.framebuffers_.size());
	
	VkCommandBufferAllocateInfo allocInfo = vulkan_util::make_command_buffer_allocate_info(graphics_.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, device_->swapchain.framebuffers_.size());

	vulkan_util::check_vulkan_result(
		vkAllocateCommandBuffers(device_->device, &allocInfo, graphics_.command_buffers.data()),
		"Failed to create command buffers."
	);

	for (int i = 0; i < graphics_.command_buffers.size(); ++i) {
		
		VkCommandBufferBeginInfo beginInfo = vulkan_util::make_command_buffer_begin_info();

		vkBeginCommandBuffer(graphics_.command_buffers[i], &beginInfo);

		
		std::vector<VkClearValue> clearValues(2);
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkRenderPassBeginInfo renderPassBeginInfo = vulkan_util::make_render_pass_begin_info(
			graphics_.render_pass,
			device_->swapchain.framebuffers_[i],
			{ 0, 0 },
			device_->swapchain.extent_,
			clearValues
		);

		
		vkCmdBeginRenderPass(graphics_.command_buffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		
		vkCmdBindPipeline(graphics_.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_.graphics_pipeline);

		for (int b = 0; b < graphics_.geometry_buffers.size(); ++b) {
			geometry_buffer& geomBuffer = graphics_.geometry_buffers[b];

			
			VkBuffer vertexBuffers[] = { geomBuffer.vertex_buffer, geomBuffer.vertex_buffer };
			VkDeviceSize offsets[] = { geomBuffer.buffer_layout.vertex_buffer_offsets.at(POSITION), geomBuffer.buffer_layout.vertex_buffer_offsets.at(NORMAL) };
			vkCmdBindVertexBuffers(graphics_.command_buffers[i], 0, 2, vertexBuffers, offsets);

			
			vkCmdBindIndexBuffer(graphics_.command_buffers[i], geomBuffer.vertex_buffer, geomBuffer.buffer_layout.vertex_buffer_offsets.at(INDEX), VK_INDEX_TYPE_UINT16);

			
			vkCmdBindDescriptorSets(graphics_.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_.pipeline_layout, 0, 1, &graphics_.descriptor_sets, 0, nullptr);

			
			vkCmdDrawIndexed(graphics_.command_buffers[i], scene_->meshesData[b]->attrib_info.at(INDEX).count, 1, 0, 0, 0);
		}

		
		vkCmdEndRenderPass(graphics_.command_buffers[i]);

		
		vulkan_util::check_vulkan_result(
			vkEndCommandBuffer(graphics_.command_buffers[i]),
			"Failed to record command buffers"
		);
	}

	return VK_SUCCESS;
}

VkResult vulkan_renderer::prepare_uniforms()
{
	VkDeviceSize bufferSize = sizeof(graphics_uniform_buffer_object);
	VkDeviceSize memoryOffset = 0;
	device_->create_buffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		graphics_.uniform_staging_buffer
	);

	
	device_->create_memory(
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		graphics_.uniform_staging_buffer,
		graphics_.uniform_staging_buffer_memory
	);

	
	vkBindBufferMemory(device_->device, graphics_.uniform_staging_buffer, graphics_.uniform_staging_buffer_memory, memoryOffset);

	device_->create_buffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		graphics_.uniform_buffer
	);

	
	device_->create_memory(
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		graphics_.uniform_buffer,
		graphics_.uniform_buffer_memory
	);

	
	vkBindBufferMemory(device_->device, graphics_.uniform_buffer, graphics_.uniform_buffer_memory, memoryOffset);

	return VK_SUCCESS;
}

VkResult vulkan_renderer::prepare_vertex_buffers()
{
	graphics_.geometry_buffers.clear();

	for (mesh_data* geomData : scene_->meshesData) {
		geometry_buffer geomBuffer;

		std::vector<unsigned char>& indexData = geomData->vertex_data.at(INDEX);
		VkDeviceSize indexBufferSize = sizeof(indexData[0]) * indexData.size();
		VkDeviceSize indexBufferOffset = 0;
		std::vector<unsigned char>& positionData = geomData->vertex_data.at(POSITION);
		VkDeviceSize positionBufferSize = sizeof(positionData[0]) * positionData.size();
		VkDeviceSize positionBufferOffset = indexBufferSize;
		std::vector<unsigned char>& normalData = geomData->vertex_data.at(NORMAL);
		VkDeviceSize normalBufferSize = sizeof(normalData[0]) * normalData.size();
		VkDeviceSize normalBufferOffset = positionBufferOffset + positionBufferSize;

		VkDeviceSize bufferSize = indexBufferSize + positionBufferSize + normalBufferSize;
		geomBuffer.buffer_layout.vertex_buffer_offsets.insert(std::make_pair(INDEX, indexBufferOffset));
		geomBuffer.buffer_layout.vertex_buffer_offsets.insert(std::make_pair(POSITION, positionBufferOffset));
		geomBuffer.buffer_layout.vertex_buffer_offsets.insert(std::make_pair(NORMAL, normalBufferOffset));

		
		
		
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		device_->create_buffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			stagingBuffer
		);

		
		device_->create_memory(
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		
		VkDeviceSize memoryOffset = 0;
		vkBindBufferMemory(device_->device, stagingBuffer, stagingBufferMemory, memoryOffset);

		
		void* data;
		vkMapMemory(device_->device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indexData.data(), static_cast<size_t>(indexBufferSize));
		memcpy((unsigned char*)data + positionBufferOffset, positionData.data(), static_cast<size_t>(positionBufferSize));
		memcpy((unsigned char*)data + normalBufferOffset, normalData.data(), static_cast<size_t>(normalBufferSize));
		vkUnmapMemory(device_->device, stagingBufferMemory);

		

		device_->create_buffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			geomBuffer.vertex_buffer
		);

		
		device_->create_memory(
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			geomBuffer.vertex_buffer,
			geomBuffer.vertex_buffer_memory
		);

		
		vkBindBufferMemory(device_->device, geomBuffer.vertex_buffer, geomBuffer.vertex_buffer_memory, memoryOffset);

		
		device_->copy_buffer(
			graphics_.graphics_queue,
			graphics_.command_pool,
			geomBuffer.vertex_buffer,
			stagingBuffer,
			bufferSize
		);

		
		vkDestroyBuffer(device_->device, stagingBuffer, nullptr);
		vkFreeMemory(device_->device, stagingBufferMemory, nullptr);

		graphics_.geometry_buffers.push_back(geomBuffer);
	}
	return VK_SUCCESS;
}

void vulkan_renderer::prepare_textures()
{
}

void vulkan_renderer::update()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float timeSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	graphics_uniform_buffer_object ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), timeSeconds * glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));
	ubo.view = glm::mat4(1);
	ubo.proj = glm::mat4(1);

	
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device_->device, graphics_.uniform_staging_buffer_memory, 0, sizeof(graphics_uniform_buffer_object), 0, &data);
	memcpy(data, &ubo, sizeof(graphics_uniform_buffer_object));
	vkUnmapMemory(device_->device, graphics_.uniform_staging_buffer_memory);

	device_->copy_buffer(
		graphics_.graphics_queue,
		graphics_.command_pool,
		graphics_.uniform_buffer,
		graphics_.uniform_staging_buffer,
		sizeof(graphics_uniform_buffer_object));
}

void vulkan_renderer::render()
{
	
	uint32_t imageIndex;
	vkAcquireNextImageKHR(
		device_->device,
		device_->swapchain.swapchain_,
		UINT64_MAX, 
		image_available_semaphore_,
		VK_NULL_HANDLE,
		&imageIndex
	);

	
	std::vector<VkSemaphore> waitSemaphores = { image_available_semaphore_ };
	std::vector<VkSemaphore> signalSemaphores = { render_finished_semaphore_ };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = vulkan_util::make_submit_info(
		waitSemaphores,
		signalSemaphores,
		waitStages,
		graphics_.command_buffers[imageIndex]
	);

	
	vulkan_util::check_vulkan_result(
		vkQueueSubmit(graphics_.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE),
		"Failed to submit queue"
	);

	
	std::vector<VkSwapchainKHR> swapchains = { device_->swapchain.swapchain_ };
	VkPresentInfoKHR presentInfo = vulkan_util::make_present_info_khr(
		signalSemaphores,
		swapchains,
		&imageIndex
	);

	vkQueuePresentKHR(graphics_.graphics_queue, &presentInfo);
}
