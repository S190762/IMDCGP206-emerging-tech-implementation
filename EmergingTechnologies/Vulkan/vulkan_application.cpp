#include "vulkan_application.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <set>
#include <SDL_Vulkan.h>

void vulkan_application::run()
{
	init_window();
	init_vulkan();
	main_loop();
	cleanup();
}

void vulkan_application::init_window()
{
	SDL_Init(SDL_INIT_VIDEO); //Initialize SDL video component
	//create a SDL window, that is centered and supports Vulkan
	sdl_window_ = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width_, height_,
	                               SDL_WINDOW_VULKAN);
	SDL_SetWindowResizable(sdl_window_, SDL_TRUE); //Enable window resizing
}

void vulkan_application::init_vulkan()
{
	create_instance();
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swap_chain();
	create_image_views();
	create_render_pass();
	create_descriptor_set_layout();
	create_graphics_pipeline();
	create_framebuffers();
	create_command_pool();
	create_vertex_buffer();
	create_index_buffer();
	create_uniform_buffer();
	create_descriptor_pool();
	create_descriptor_set();
	create_command_buffers();
	create_semaphores();
}

void vulkan_application::main_loop()
{
	auto running = true;
	while (running)
	{
		SDL_Event event;
		//while there are events in the sdl queue
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				running = false;
			}
			else if (event.type == SDL_WINDOWEVENT && (event.window.type == SDL_WINDOWEVENT_RESIZED || event.window.type ==
				SDL_WINDOWEVENT_SIZE_CHANGED))
			{
				//if the window has been resized or maximised, we need to recreate the swap chain using
				//the new window height and width
				recreate_swap_chain();
			}
		}
		//resend the uniform buffer data to the GPU with the new data
		update_uniform_buffer();
		//draw a frame
		draw_frame();
	}
	//wait for the device to be idle before rendering a new frame
	vkDeviceWaitIdle(logical_device_);
}

void vulkan_application::cleanup_swap_chain()
{
	//destroy all framebuffers
	for (auto framebuffer : swap_chain_framebuffers_)
	{
		vkDestroyFramebuffer(logical_device_, framebuffer, nullptr);
	}

	//remove all the command buffers
	vkFreeCommandBuffers(logical_device_, command_pool_, static_cast<uint32_t>(command_buffers_.size()),
	                     command_buffers_.data());

	//destroy the graphics pipeline
	vkDestroyPipeline(logical_device_, graphics_pipeline_, nullptr);
	vkDestroyPipelineLayout(logical_device_, pipeline_layout_, nullptr);
	vkDestroyRenderPass(logical_device_, render_pass_, nullptr);

	//destroy all image views
	for (auto image_view : swap_chain_image_views_)
	{
		vkDestroyImageView(logical_device_, image_view, nullptr);
	}

	//destroy the swapchain
	vkDestroySwapchainKHR(logical_device_, swap_chain_, nullptr);
}

void vulkan_application::cleanup()
{
	cleanup_swap_chain();

	//destroy the descriptor sets
	vkDestroyDescriptorPool(logical_device_, descriptor_pool_, nullptr);
	vkDestroyDescriptorSetLayout(logical_device_, descriptor_set_layout_, nullptr);

	//destroy the uniform buffer and free its memory on the gpu
	vkDestroyBuffer(logical_device_, uniform_buffer_, nullptr);
	vkFreeMemory(logical_device_, uniform_buffer_memory_, nullptr);

	//destroy the index buffer and free its memory on the gpu
	vkDestroyBuffer(logical_device_, index_buffer_, nullptr);
	vkFreeMemory(logical_device_, index_buffer_memory_, nullptr);

	//destroy the vertex buffer and free its memory on the gpu
	vkDestroyBuffer(logical_device_, vertex_buffer_, nullptr);
	vkFreeMemory(logical_device_, vertex_buffer_memory_, nullptr);

	//destroy the synchronisation semaphores
	vkDestroySemaphore(logical_device_, render_finished_semaphore_, nullptr);
	vkDestroySemaphore(logical_device_, image_available_semaphore_, nullptr);

	//destroy the command pool
	vkDestroyCommandPool(logical_device_, command_pool_, nullptr);

	//destroy the device
	vkDestroyDevice(logical_device_, nullptr);

	//destroy the surface and vulkan instance
	vkDestroySurfaceKHR(vulkan_instance_, vulkan_surface_, nullptr);
	vkDestroyInstance(vulkan_instance_, nullptr);

	//exit sdl
	SDL_DestroyWindow(sdl_window_);
	SDL_Quit();
}

void vulkan_application::recreate_swap_chain()
{
	//obtain the new window width and height
	int w, h;
	SDL_GetWindowSize(sdl_window_, &w, &h);
	width_ = w;
	height_ = h;
	if (width_ == 0 || height_ == 0) return;

	//wait for the gpu to be idle
	vkDeviceWaitIdle(logical_device_);

	cleanup_swap_chain();

	create_swap_chain();
	create_image_views();
	create_render_pass();
	create_graphics_pipeline();
	create_framebuffers();
	create_command_buffers();
}

void vulkan_application::create_instance()
{
	//A structure to contain information about the application
	//This is used by the driver to optimise for certain engines
	VkApplicationInfo vk_application_info = {};
	vk_application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_application_info.pApplicationName = "Hello Triangle"; //the name of our application
	vk_application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0); //this is v1.0 of our app
	vk_application_info.pEngineName = "No Engine"; //the name of our engine
	vk_application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0); //this is v1.0 of our engine
	vk_application_info.apiVersion = VK_API_VERSION_1_0; //require vulkan 1.0

	//A structure used t
	VkInstanceCreateInfo vk_instance_create_info = {};
	vk_instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vk_instance_create_info.pApplicationInfo = &vk_application_info; //ptr to the application info

	auto extensions = get_required_extensions(); //obtain the device extensions we need for this platform
	vk_instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	//the number of extensions to enable
	vk_instance_create_info.ppEnabledExtensionNames = extensions.data();
	//ptr to the memory location of the extensions vector

	vk_instance_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
	vk_instance_create_info.ppEnabledLayerNames = validation_layers.data();
	//ptr to the memory location of the validation layers vector

	//Create the instance
	if (vkCreateInstance(&vk_instance_create_info, nullptr, &vulkan_instance_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void vulkan_application::create_surface()
{
	//Cal the SDL_Vulkan create surface function
	SDL_Vulkan_CreateSurface(sdl_window_, vulkan_instance_, &vulkan_surface_);
}

void vulkan_application::pick_physical_device()
{
	//obtain the number of physical devices on the system
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(vulkan_instance_, &device_count, nullptr);

	//if this returned 0 then there are no suitable devices
	if (device_count == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	//initalize the vector with the number of devices, and obtain them
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(vulkan_instance_, &device_count, devices.data());

	//Find a device which is suitable
	for (const auto& device : devices)
	{
		if (is_device_suitable(device))
		{
			physical_device_ = device;
			break;
		}
	}

	//If the physical device was not set, then a suitable GPU was not found
	if (physical_device_ == nullptr)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void vulkan_application::create_logical_device()
{
	//obtain the gfx and present queue id's
	const auto indices = find_queue_families(physical_device_);

	//Vulkan needs to be passed an array of queue create infos, because multiple queues may be
	//use by a gpu.
	std::vector<VkDeviceQueueCreateInfo> vk_device_queue_create_infos;
	std::set<int> unique_queue_families = {indices.graphics_family, indices.present_family};

	//Since multiple sets of queues can be used, it is required to set a priority for each of these
	//however since only 1 set of queues will be used, the priority is set to 1
	auto queue_priority = 1.0F;
	//Loop through the queue family infos
	for (auto queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo vk_device_queue_create_info = {};
		vk_device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		vk_device_queue_create_info.queueFamilyIndex = queue_family; //the index of the queue family
		vk_device_queue_create_info.queueCount = 1; //there is 1 queue
		vk_device_queue_create_info.pQueuePriorities = &queue_priority; //pass the priortiy
		vk_device_queue_create_infos.push_back(vk_device_queue_create_info);
	}

	//define what device features we wish to enable
	//we are not using any other the the default
	VkPhysicalDeviceFeatures vk_physical_device_features = {};

	//Now define the device to create
	VkDeviceCreateInfo vk_device_create_info = {};
	vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//pass the queue create info's
	vk_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(vk_device_queue_create_infos.size());
	vk_device_create_info.pQueueCreateInfos = vk_device_queue_create_infos.data();

	//pass the enabled features
	vk_device_create_info.pEnabledFeatures = &vk_physical_device_features;

	//pass the enabled device extensions
	vk_device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	vk_device_create_info.ppEnabledExtensionNames = device_extensions.data();

	//pass the enabled validation layers
	vk_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
	vk_device_create_info.ppEnabledLayerNames = validation_layers.data();

	//create the device
	if (vkCreateDevice(physical_device_, &vk_device_create_info, nullptr, &logical_device_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	//obtain the queue's from the device for the specified id's
	vkGetDeviceQueue(logical_device_, indices.graphics_family, 0, &graphics_queue_);
	vkGetDeviceQueue(logical_device_, indices.present_family, 0, &present_queue_);
}

void vulkan_application::create_swap_chain()
{
	const auto swap_chain_support = query_swap_chain_support(physical_device_); //obtain the swap chain data
	const auto vk_surface_format_khr = choose_swap_surface_format(swap_chain_support.formats); //choose the surface format
	const auto vk_present_mode_khr = choose_swap_present_mode(swap_chain_support.present_modes); //choose the present mode
	const auto vk_extent2_d = choose_swap_extent(swap_chain_support.capabilities); //choose the extent

	//obtain the number of images the device has
	auto image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
	{
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	//define the swapchain to create
	VkSwapchainCreateInfoKHR vk_swapchain_create_info_khr = {};
	vk_swapchain_create_info_khr.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	vk_swapchain_create_info_khr.surface = vulkan_surface_; //the surface to render to

	vk_swapchain_create_info_khr.minImageCount = image_count; //the number of images
	vk_swapchain_create_info_khr.imageFormat = vk_surface_format_khr.format; //the format of each image
	vk_swapchain_create_info_khr.imageColorSpace = vk_surface_format_khr.colorSpace; //the color space of each image
	vk_swapchain_create_info_khr.imageExtent = vk_extent2_d; //the dimensions of the image
	vk_swapchain_create_info_khr.imageArrayLayers = 1; //the number of layers
	vk_swapchain_create_info_khr.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //use the image for color

	//obtain the queue family id's and create a c array for use with vulkan
	const auto indices = find_queue_families(physical_device_);
	uint32_t queue_family_indices[] = {
		static_cast<uint32_t>(indices.graphics_family), static_cast<uint32_t>(indices.present_family)
	};

	//if the graphics queue is not the same as the present queue
	//enmable concurrent sharing of images
	if (indices.graphics_family != indices.present_family)
	{
		vk_swapchain_create_info_khr.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		vk_swapchain_create_info_khr.queueFamilyIndexCount = 2;
		vk_swapchain_create_info_khr.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		//else enable exclusive sharing
		vk_swapchain_create_info_khr.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	vk_swapchain_create_info_khr.preTransform = swap_chain_support.capabilities.currentTransform;
	vk_swapchain_create_info_khr.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	vk_swapchain_create_info_khr.presentMode = vk_present_mode_khr;
	vk_swapchain_create_info_khr.clipped = VK_TRUE;

	//create the swapchain
	if (vkCreateSwapchainKHR(logical_device_, &vk_swapchain_create_info_khr, nullptr, &swap_chain_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	//obtain the images from the swapchain, intialize the image vector and fill it with the images
	vkGetSwapchainImagesKHR(logical_device_, swap_chain_, &image_count, nullptr);
	swap_chain_images_.resize(image_count);
	vkGetSwapchainImagesKHR(logical_device_, swap_chain_, &image_count, swap_chain_images_.data());

	//set the format and extent of the swapchain
	swap_chain_image_format_ = vk_surface_format_khr.format;
	swap_chain_extent_ = vk_extent2_d;
}

void vulkan_application::create_image_views()
{
	//initalize the image views vector with the number of images
	swap_chain_image_views_.resize(swap_chain_images_.size());

	//loop through all the images and create a view for them
	for (size_t i = 0; i < swap_chain_images_.size(); i++)
	{
		VkImageViewCreateInfo vk_image_view_create_info = {};
		vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vk_image_view_create_info.image = swap_chain_images_[i]; //the image that this view will use
		vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; //2D image
		vk_image_view_create_info.format = swap_chain_image_format_; //the image format 
		vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
		vk_image_view_create_info.subresourceRange.levelCount = 1;
		vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
		vk_image_view_create_info.subresourceRange.layerCount = 1;

		//create the image view
		if (vkCreateImageView(logical_device_, &vk_image_view_create_info, nullptr, &swap_chain_image_views_[i]) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void vulkan_application::create_render_pass()
{
	//Define the render pass attachment, for color
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = swap_chain_image_format_;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; //1x multisample
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Define the reference to the color attachment
	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Define the subpasses for this render pass, which is the color attachment
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //this is a graphics pipeline
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	//Define the subpass dependency, which is color attachment
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//Define the render pass to create
	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = 1;
	render_pass_create_info.pAttachments = &color_attachment; //1 attachment, color
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass; //1 subpass
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency; //1 dependency

	//Create the render pass
	if (vkCreateRenderPass(logical_device_, &render_pass_create_info, nullptr, &render_pass_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void vulkan_application::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding vk_descriptor_set_layout_binding = {};
	vk_descriptor_set_layout_binding.binding = 0;
	vk_descriptor_set_layout_binding.descriptorCount = 1;
	vk_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; //this is a uniform buffer
	vk_descriptor_set_layout_binding.pImmutableSamplers = nullptr;
	vk_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //used in the vertex shader

	VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info = {};
	vk_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	vk_descriptor_set_layout_create_info.bindingCount = 1;
	vk_descriptor_set_layout_create_info.pBindings = &vk_descriptor_set_layout_binding;

	//create the descriptor set layout
	if (vkCreateDescriptorSetLayout(logical_device_, &vk_descriptor_set_layout_create_info, nullptr,
	                                &descriptor_set_layout_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void vulkan_application::create_graphics_pipeline()
{
	//read the SPIR-V vertex and fragment shaders
	const auto vert_shader_code = read_file("shaders/vert.spv");
	const auto frag_shader_code = read_file("shaders/frag.spv");

	//create vulkan shader modules for each shader
	const auto vert_shader_module = create_shader_module(vert_shader_code);
	const auto frag_shader_module = create_shader_module(frag_shader_code);

	//define the vertex shader stage
	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT; //this is a vertex shader
	vert_shader_stage_info.module = vert_shader_module; //the shader module for vertex
	vert_shader_stage_info.pName = "main"; //the name of the main function of the vertex shader

	//define the fragment shader stage
	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT; //this is a fragment shader
	frag_shader_stage_info.module = frag_shader_module; //the shader module for fragment
	frag_shader_stage_info.pName = "main"; //the name of the main function of the fragment shader

	//define all the shader stages for this pipeline
	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

	//define the vertex input stage
	//this is where we tell the gpu what the data is we are sending it
	VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info = {};
	vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	//get the binding description and attribute descriptions for the vertex data
	auto vk_vertex_input_binding_description = vertex::getBindingDescription();
	auto vk_vertex_input_attribute_descriptions = vertex::get_attribute_descriptions();

	//pass these to vulkan
	vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
	vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(
		vk_vertex_input_attribute_descriptions.size());
	vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
	vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_vertex_input_attribute_descriptions.
		data();

	//input assembly stage
	//this is where the data from the previous stage is assembled into vertices
	VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info = {};
	vk_pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vk_pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //Triangle List 
	vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	//the viewport or "render area" of the window
	//x 0, y 0, width = window width, height = window height
	VkViewport vk_viewport = {};
	vk_viewport.x = 0.0F;
	vk_viewport.y = 0.0F;
	vk_viewport.width = static_cast<float>(swap_chain_extent_.width);
	vk_viewport.height = static_cast<float>(swap_chain_extent_.height);
	//enable z buffer
	vk_viewport.minDepth = 0.0F;
	vk_viewport.maxDepth = 1.0F;

	//enable scissor mask, which is used so that vulkan doesnt render anything
	//outside of the viewport area
	VkRect2D vk_rect2_d = {};
	vk_rect2_d.offset = {0, 0};
	vk_rect2_d.extent = swap_chain_extent_;

	//define the viewport and scissor stage of the pipeline
	VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info = {};
	vk_pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vk_pipeline_viewport_state_create_info.viewportCount = 1;
	vk_pipeline_viewport_state_create_info.pViewports = &vk_viewport;
	vk_pipeline_viewport_state_create_info.scissorCount = 1;
	vk_pipeline_viewport_state_create_info.pScissors = &vk_rect2_d;

	//define the rasterizer stage, which takes the vertices and produces an image
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //fill polygons (not wireframe)
	rasterizer.lineWidth = 1.0F;
	//the width of lines between each vertex, set to 1 but doesnt have an effect unless on certain devices
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //back face culling
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //ccw vertex drawing
	rasterizer.depthBiasEnable = VK_FALSE;

	//define the multisampling stage, which performs multisampling to remove jagged edges
	//we will only enable 1x multisampling as any other requires a lot more code
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //1x multisample

	//define the color blending stage, this is where color is applied to the image
	VkPipelineColorBlendAttachmentState vk_pipeline_color_blend_attachment_state = {};
	vk_pipeline_color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	vk_pipeline_color_blend_attachment_state.blendEnable = VK_FALSE; //blending is disabled

	VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info = {};
	vk_pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	vk_pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
	vk_pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
	vk_pipeline_color_blend_state_create_info.attachmentCount = 1;
	vk_pipeline_color_blend_state_create_info.pAttachments = &vk_pipeline_color_blend_attachment_state;
	vk_pipeline_color_blend_state_create_info.blendConstants[0] = 0.0F;
	vk_pipeline_color_blend_state_create_info.blendConstants[1] = 0.0F;
	vk_pipeline_color_blend_state_create_info.blendConstants[2] = 0.0F;
	vk_pipeline_color_blend_state_create_info.blendConstants[3] = 0.0F;

	//Finally the pipeline layout is created from the descriptor sets
	VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info = {};
	vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vk_pipeline_layout_create_info.setLayoutCount = 1;
	vk_pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout_;

	if (vkCreatePipelineLayout(logical_device_, &vk_pipeline_layout_create_info, nullptr, &pipeline_layout_) != VK_SUCCESS
	)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//Define the graphics pipeline and its contents
	VkGraphicsPipelineCreateInfo vk_graphics_pipeline_create_info = {};
	vk_graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	//this is a graphics pipeline
	vk_graphics_pipeline_create_info.stageCount = 2; //we have a vertex and a fragment shader so there are 2 stages
	vk_graphics_pipeline_create_info.pStages = shader_stages;
	//pass the various stages of the pipeline
	vk_graphics_pipeline_create_info.pVertexInputState = &vk_pipeline_vertex_input_state_create_info;
	vk_graphics_pipeline_create_info.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info;
	vk_graphics_pipeline_create_info.pViewportState = &vk_pipeline_viewport_state_create_info;
	vk_graphics_pipeline_create_info.pRasterizationState = &rasterizer;
	vk_graphics_pipeline_create_info.pMultisampleState = &multisampling;
	vk_graphics_pipeline_create_info.pColorBlendState = &vk_pipeline_color_blend_state_create_info;
	vk_graphics_pipeline_create_info.layout = pipeline_layout_;
	vk_graphics_pipeline_create_info.renderPass = render_pass_; //render pass reference
	vk_graphics_pipeline_create_info.subpass = 0;
	vk_graphics_pipeline_create_info.basePipelineHandle = nullptr;

	//create the pipeline
	if (vkCreateGraphicsPipelines(logical_device_, nullptr, 1, &vk_graphics_pipeline_create_info, nullptr,
	                              &graphics_pipeline_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	//Delete the shader modules as they are now no longer needed as they are attached to the pipeline
	vkDestroyShaderModule(logical_device_, frag_shader_module, nullptr);
	vkDestroyShaderModule(logical_device_, vert_shader_module, nullptr);
}

void vulkan_application::create_framebuffers()
{
	swap_chain_framebuffers_.resize(swap_chain_image_views_.size());

	//create a framebuffer for each image view
	for (size_t i = 0; i < swap_chain_image_views_.size(); i++)
	{
		//attach the image view to this framebuffer
		VkImageView attachments[] = {
			swap_chain_image_views_[i]
		};

		VkFramebufferCreateInfo vk_framebuffer_create_info = {};
		vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		vk_framebuffer_create_info.renderPass = render_pass_;
		vk_framebuffer_create_info.attachmentCount = 1;
		vk_framebuffer_create_info.pAttachments = attachments;
		vk_framebuffer_create_info.width = swap_chain_extent_.width; //the width is the same as the swapchain
		vk_framebuffer_create_info.height = swap_chain_extent_.height; //the height is the same as the swapchain
		vk_framebuffer_create_info.layers = 1;

		//create the framebuffer
		if (vkCreateFramebuffer(logical_device_, &vk_framebuffer_create_info, nullptr, &swap_chain_framebuffers_[i]) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void vulkan_application::create_command_pool()
{
	const auto indices = find_queue_families(physical_device_);

	//set the id of the graphics queue to use
	VkCommandPoolCreateInfo vk_command_pool_create_info = {};
	vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	vk_command_pool_create_info.queueFamilyIndex = indices.graphics_family;

	//create the command pool
	if (vkCreateCommandPool(logical_device_, &vk_command_pool_create_info, nullptr, &command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics command pool!");
	}
}

void vulkan_application::create_vertex_buffer()
{
	//define the size of the memory block, which is the size of a vertex multiplied by the size of the vertex struct
	const auto buffer_size = sizeof(vertices[0]) * vertices.size();

	//create a staging buffer in local memory, which will be used to upload data to the GPU
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
	              staging_buffer_memory);

	//now copy the vertex data to the staging buffer
	void* data;
	vkMapMemory(logical_device_, staging_buffer_memory, 0, buffer_size, 0, &data);
	//obtain the memory location for writing
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size)); //copy the data to this memory location
	vkUnmapMemory(logical_device_, staging_buffer_memory); //close the memory location for writing

	//create the vertex buffer
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer_, vertex_buffer_memory_);
	//copy the data from the staging buffer to the vertex buffer on the GPU
	copy_buffer(staging_buffer, vertex_buffer_, buffer_size);

	//now destroy the staging buffer and free its memory
	vkDestroyBuffer(logical_device_, staging_buffer, nullptr);
	vkFreeMemory(logical_device_, staging_buffer_memory, nullptr);
}

void vulkan_application::create_index_buffer()
{
	const auto buffer_size = sizeof(indices[0]) * indices.size();

	//create a staging buffer in local memory, which will be used to upload data to the GPU
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer,
	              staging_buffer_memory);

	//now copy the vertex data to the staging buffer
	void* data;
	vkMapMemory(logical_device_, staging_buffer_memory, 0, buffer_size, 0, &data);
	//obtain the memory location for writing
	memcpy(data, indices.data(), static_cast<size_t>(buffer_size)); //copy the data to this memory location
	vkUnmapMemory(logical_device_, staging_buffer_memory); //close the memory location for writing

	//create the index buffer
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer_, index_buffer_memory_);
	//copy the data from the staging buffer to the index buffer on the GPU
	copy_buffer(staging_buffer, index_buffer_, buffer_size);

	//now destroy the staging buffer and free its memory
	vkDestroyBuffer(logical_device_, staging_buffer, nullptr);
	vkFreeMemory(logical_device_, staging_buffer_memory, nullptr);
}

void vulkan_application::create_uniform_buffer()
{
	const auto buffer_size = sizeof(uniform_buffer_object);
	create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer_,
	              uniform_buffer_memory_);
}

void vulkan_application::create_descriptor_pool()
{
	VkDescriptorPoolSize vk_descriptor_pool_size = {};
	vk_descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vk_descriptor_pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info = {};
	vk_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	vk_descriptor_pool_create_info.poolSizeCount = 1;
	vk_descriptor_pool_create_info.pPoolSizes = &vk_descriptor_pool_size;
	vk_descriptor_pool_create_info.maxSets = 1;

	if (vkCreateDescriptorPool(logical_device_, &vk_descriptor_pool_create_info, nullptr, &descriptor_pool_) != VK_SUCCESS
	)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void vulkan_application::create_descriptor_set()
{
	VkDescriptorSetLayout layouts[] = {descriptor_set_layout_};
	VkDescriptorSetAllocateInfo vk_descriptor_set_allocate_info = {};
	vk_descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	vk_descriptor_set_allocate_info.descriptorPool = descriptor_pool_;
	vk_descriptor_set_allocate_info.descriptorSetCount = 1;
	vk_descriptor_set_allocate_info.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(logical_device_, &vk_descriptor_set_allocate_info, &descriptor_set_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo vk_descriptor_buffer_info = {};
	vk_descriptor_buffer_info.buffer = uniform_buffer_;
	vk_descriptor_buffer_info.offset = 0;
	vk_descriptor_buffer_info.range = sizeof(uniform_buffer_object); //the size of the uniform data that will be sent

	VkWriteDescriptorSet vk_write_descriptor_set = {};
	vk_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vk_write_descriptor_set.dstSet = descriptor_set_;
	vk_write_descriptor_set.dstBinding = 0;
	vk_write_descriptor_set.dstArrayElement = 0;
	vk_write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vk_write_descriptor_set.descriptorCount = 1;
	vk_write_descriptor_set.pBufferInfo = &vk_descriptor_buffer_info;

	vkUpdateDescriptorSets(logical_device_, 1, &vk_write_descriptor_set, 0, nullptr);
}

void vulkan_application::create_buffer(const VkDeviceSize size, const VkBufferUsageFlags usage,
                                       const VkMemoryPropertyFlags properties, VkBuffer& buffer,
                                       VkDeviceMemory& buffer_memory) const
{
	VkBufferCreateInfo vk_buffer_create_info = {};
	vk_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vk_buffer_create_info.size = size;
	vk_buffer_create_info.usage = usage;
	vk_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logical_device_, &vk_buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	//obtaint he memory requirements for the buffer
	VkMemoryRequirements vk_memory_requirements;
	vkGetBufferMemoryRequirements(logical_device_, buffer, &vk_memory_requirements);

	VkMemoryAllocateInfo vk_memory_allocate_info = {};
	vk_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vk_memory_allocate_info.allocationSize = vk_memory_requirements.size; //the size of memory we need
	vk_memory_allocate_info.memoryTypeIndex = find_memory_type(vk_memory_requirements.memoryTypeBits, properties);
	//the type of memory

	//allocate the memory for this buffer on the GPU
	if (vkAllocateMemory(logical_device_, &vk_memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	//bind the memory on the GPU
	vkBindBufferMemory(logical_device_, buffer, buffer_memory, 0);
}

void vulkan_application::copy_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer,
                                     const VkDeviceSize size) const
{
	//In order to copy data from a buffer to another, we need to execute the commands on the GPU
	//therefore a command buffer is required
	VkCommandBufferAllocateInfo vk_command_buffer_allocate_info = {};
	vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vk_command_buffer_allocate_info.commandPool = command_pool_; //use the previously specified command pool
	vk_command_buffer_allocate_info.commandBufferCount = 1;

	//obtain a command buffer to use from the GPU
	VkCommandBuffer vk_command_buffer;
	vkAllocateCommandBuffers(logical_device_, &vk_command_buffer_allocate_info, &vk_command_buffer);

	//begin recording the commands
	VkCommandBufferBeginInfo vk_command_buffer_begin_info = {};
	vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vk_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	//this command will be submitted once

	vkBeginCommandBuffer(vk_command_buffer, &vk_command_buffer_begin_info);

	//define the size of data to copy
	VkBufferCopy vk_buffer_copy = {};
	vk_buffer_copy.size = size;
	//perform the copying
	vkCmdCopyBuffer(vk_command_buffer, src_buffer, dst_buffer, 1, &vk_buffer_copy);

	//end recording
	vkEndCommandBuffer(vk_command_buffer);

	//submit these commands to the graphics queue and execute
	VkSubmitInfo vk_submit_info = {};
	vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vk_submit_info.commandBufferCount = 1;
	vk_submit_info.pCommandBuffers = &vk_command_buffer;

	vkQueueSubmit(graphics_queue_, 1, &vk_submit_info, nullptr);
	//wait for the graphics queue to be idle which signifies the command was complete
	vkQueueWaitIdle(graphics_queue_);

	//destroy the command buffer
	vkFreeCommandBuffers(logical_device_, command_pool_, 1, &vk_command_buffer);
}

uint32_t vulkan_application::find_memory_type(const uint32_t type_filter, const VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void vulkan_application::create_command_buffers()
{
	//we need the same number of command buffers as there are framebuffers
	command_buffers_.resize(swap_chain_framebuffers_.size());

	//allocate command buffers to the command pool
	VkCommandBufferAllocateInfo vk_command_buffer_allocate_info = {};
	vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vk_command_buffer_allocate_info.commandPool = command_pool_;
	vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vk_command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

	if (vkAllocateCommandBuffers(logical_device_, &vk_command_buffer_allocate_info, command_buffers_.data()) != VK_SUCCESS
	)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	//For each command buffer, record the commands that will be executed by the GPU each frame
	//This is where the drawing code goes
	for (size_t i = 0; i < command_buffers_.size(); i++)
	{
		//begin recording command
		VkCommandBufferBeginInfo vk_command_buffer_begin_info = {};
		vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vk_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		//these commands will be executed once
		vkBeginCommandBuffer(command_buffers_[i], &vk_command_buffer_begin_info);

		//define the render pass, framebuffer and render area
		VkRenderPassBeginInfo vk_render_pass_begin_info = {};
		vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vk_render_pass_begin_info.renderPass = render_pass_;
		vk_render_pass_begin_info.framebuffer = swap_chain_framebuffers_[i];
		vk_render_pass_begin_info.renderArea.offset = {0, 0};
		vk_render_pass_begin_info.renderArea.extent = swap_chain_extent_;

		//set the clear color 
		VkClearValue vk_clear_value = {0.0F, 0.0F, 0.0F, 1.0F};
		vk_render_pass_begin_info.clearValueCount = 1;
		vk_render_pass_begin_info.pClearValues = &vk_clear_value;

		//begin the render pass
		vkCmdBeginRenderPass(command_buffers_[i], &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		//bind the graphics pipeline
		vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

		//define the vertex buffers required
		VkBuffer vertex_buffers[] = {vertex_buffer_};
		VkDeviceSize offsets[] = {0};
		//bind the vertex buffers
		vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertex_buffers, offsets);

		//bind the index buffer
		vkCmdBindIndexBuffer(command_buffers_[i], index_buffer_, 0, VK_INDEX_TYPE_UINT16);

		//bind the descriptor sets (uniform buffers)
		vkCmdBindDescriptorSets(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1,
		                        &descriptor_set_, 0, nullptr);

		//draw the vertices index using the indices
		vkCmdDrawIndexed(command_buffers_[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		//end the rennder pass
		vkCmdEndRenderPass(command_buffers_[i]);

		//end command recording
		if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void vulkan_application::create_semaphores()
{
	VkSemaphoreCreateInfo vk_semaphore_create_info = {};
	vk_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//create the semaphores
	if (vkCreateSemaphore(logical_device_, &vk_semaphore_create_info, nullptr, &image_available_semaphore_) != VK_SUCCESS
		||
		vkCreateSemaphore(logical_device_, &vk_semaphore_create_info, nullptr, &render_finished_semaphore_) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores!");
	}
}

void vulkan_application::update_uniform_buffer() const
{
	//obtain a delta time value
	static auto time_point = std::chrono::high_resolution_clock::now();
	const auto current_time = std::chrono::high_resolution_clock::now();
	const auto time1 = std::chrono::duration<float, std::chrono::seconds::period>(current_time - time_point).count();

	//define the data to be sent to the GPU
	uniform_buffer_object ubo = {};
	ubo.model = rotate(glm::mat4(1.0F), time1 * glm::radians(90.0F), glm::vec3(0.0F, 0.0F, 1.0F));
	ubo.view = lookAt(glm::vec3(2.0F, 2.0F, 2.0F), glm::vec3(0.0F, 0.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F));
	ubo.proj = glm::perspective(glm::radians(45.0F),
	                            swap_chain_extent_.width / static_cast<float>(swap_chain_extent_.height), 0.1F, 10.0F);
	ubo.proj[1][1] *= -1; //vulkan is Y up, so the projection needs to be flipped

	//directly copy this new data to the location of the uniform buffer on the GPU's memory
	void* data;
	vkMapMemory(logical_device_, uniform_buffer_memory_, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(logical_device_, uniform_buffer_memory_);
}

void vulkan_application::draw_frame()
{
	//Obtain the ID of the image to render to next
	uint32_t image_index;
	auto result = vkAcquireNextImageKHR(logical_device_, swap_chain_, std::numeric_limits<uint64_t>::max(),
	                                    image_available_semaphore_, nullptr, &image_index);

	//if vulkan instead says that the swapchain is out of data (e.g. the window has been resized), then recreate the swap chain
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swap_chain();
		return;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	//define what will be submitted to the GPU graphics queue
	VkSubmitInfo vk_submit_info = {};
	vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//wait for an image to be available
	VkSemaphore wait_semaphores[] = {image_available_semaphore_};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	vk_submit_info.waitSemaphoreCount = 1;
	vk_submit_info.pWaitSemaphores = wait_semaphores;
	vk_submit_info.pWaitDstStageMask = wait_stages;

	//we will be submitting one command buffer to the GPU, this is the command buffer for each framebuffer (or image view)
	vk_submit_info.commandBufferCount = 1;
	vk_submit_info.pCommandBuffers = &command_buffers_[image_index];

	//wait for the render to be finished
	VkSemaphore signal_semaphores[] = {render_finished_semaphore_};
	vk_submit_info.signalSemaphoreCount = 1;
	vk_submit_info.pSignalSemaphores = signal_semaphores;

	//submit this to the graphics queue
	if (vkQueueSubmit(graphics_queue_, 1, &vk_submit_info, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//define what will be submitted to the GPU present queue
	VkPresentInfoKHR vk_present_info_khr = {};
	vk_present_info_khr.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	//wait for rendering to be finished
	vk_present_info_khr.waitSemaphoreCount = 1;
	vk_present_info_khr.pWaitSemaphores = signal_semaphores;

	//the swapchain to use
	VkSwapchainKHR vk_swapchain_khr__s[] = {swap_chain_};
	vk_present_info_khr.swapchainCount = 1;
	vk_present_info_khr.pSwapchains = vk_swapchain_khr__s;
	//the id of the image
	vk_present_info_khr.pImageIndices = &image_index;
	//submit this to the present queue (display the image)
	result = vkQueuePresentKHR(present_queue_, &vk_present_info_khr);

	//if vulkan instead says that the swapchain is out of data (e.g. the window has been resized), then recreate the swap chain
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreate_swap_chain();
	}
		//if the submission was not a success, then an image could not be displayed for some reason
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	//wait for the present queue to be idle (presentation is finished), before rendering another frame
	vkQueueWaitIdle(present_queue_);
}

VkShaderModule vulkan_application::create_shader_module(const std::vector<char>& code) const
{
	VkShaderModuleCreateInfo vk_shader_module_create_info = {};
	vk_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vk_shader_module_create_info.codeSize = code.size();
	vk_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
	//convert from char to integer data

	VkShaderModule vk_shader_module;
	if (vkCreateShaderModule(logical_device_, &vk_shader_module_create_info, nullptr, &vk_shader_module) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return vk_shader_module;
}

VkSurfaceFormatKHR vulkan_application::choose_swap_surface_format(
	const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	//if there is only 1 available swapchain format
	if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		//Then the format will be nonlinear srgb color space, and 8 bit RGBA
	}

	//otherwise, loop through the available formats and find the one that best matches what we want
	for (const auto& availableFormat : available_formats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace ==
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR vulkan_application::choose_swap_present_mode(
	const std::vector<VkPresentModeKHR> available_present_modes)
{
	auto best_mode = VK_PRESENT_MODE_FIFO_KHR; //by default, use first in, first out present mode

	for (const auto& availablePresentMode : available_present_modes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			//if mailbox is available, then we want this mode
			return availablePresentMode;
		}
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			//otherwise prefer immediate presentation if possible
			best_mode = availablePresentMode;
		}
	}

	return best_mode;
}

VkExtent2D vulkan_application::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) 
{
	//if the current extent has already been set return that
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	//obtain the window size
	int w, h;
	SDL_GetWindowSize(sdl_window_, &w, &h);
	width_ = w;
	height_ = h;

	//set the width and height
	VkExtent2D vk_extent2_d = {
		static_cast<uint32_t>(width_),
		static_cast<uint32_t>(height_)
	};

	vk_extent2_d.width = std::max(capabilities.minImageExtent.width,
	                              std::min(capabilities.maxImageExtent.width, vk_extent2_d.width));
	vk_extent2_d.height = std::max(capabilities.minImageExtent.height,
	                               std::min(capabilities.maxImageExtent.height, vk_extent2_d.height));

	return vk_extent2_d;
}

swap_chain_support_details vulkan_application::query_swap_chain_support(const VkPhysicalDevice device) const
{
	swap_chain_support_details details;
	//obtain the capabilities of the surface
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkan_surface_, &details.capabilities);

	//obtain the number of formats the surface supports
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan_surface_, &format_count, nullptr);

	//obtain what formats the surface supports
	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan_surface_, &format_count, details.formats.data());
	}

	//obtain the number of present modes the surface supports
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan_surface_, &present_mode_count, nullptr);

	//obtain what present modes the surface supports
	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan_surface_, &present_mode_count,
		                                          details.present_modes.data());
	}

	return details;
}

bool vulkan_application::is_device_suitable(const VkPhysicalDevice device) const
{
	auto indices = find_queue_families(device);

	//are the device extensions we want supported?
	const auto extensions_supported = check_device_extension_support(device);

	//is the swapchain what we need?
	auto swap_chain_adequate = false;
	if (extensions_supported)
	{
		auto swap_chain_support = query_swap_chain_support(device);
		//if the device has image formats and present modes then it is adequate
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}

	//if the extensions are supported, we have found a graphics and present queue to use, and the swapchain is what we need
	//the device is suitabel
	return indices.is_complete() && extensions_supported && swap_chain_adequate;
}

bool vulkan_application::check_device_extension_support(const VkPhysicalDevice device)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	//get the extensions supported
	std::vector<VkExtensionProperties> extension_properties(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extension_properties.data());

	//set what extensions we need supported
	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	//remove the supported extensions
	for (const auto& extension : extension_properties)
	{
		required_extensions.erase(extension.extensionName);
	}

	//if there are no extensions left then all are supported
	return required_extensions.empty();
}

queue_family_indices vulkan_application::find_queue_families(const VkPhysicalDevice device) const
{
	queue_family_indices indices;

	//obtain the number of queue families supported
	//a queue family is a set of queues (graphics, present, compute .etc)
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	//intialize the queue familiy vector and fill it with the queue families of the device
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	auto i = 0;
	for (const auto& queueFamily : queue_families)
	{
		//if the number of queues in the queue family is greater than 0 and the queue supports graphics
		//then we have found our graphics queue
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family = i;
		}

		//check that the device supports presentation, some vulkan devices might not support present
		//for example a CPU or compute hardware
		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkan_surface_, &present_support);

		//if the number of queues in the queue family is greater than 0 and the device supports presentation
		//then we have also found our presentation queue 
		if (queueFamily.queueCount > 0 && present_support)
		{
			indices.present_family = i;
		}

		//if we have now found both our graphics queue and presentation queue then we are complete
		if (indices.is_complete())
		{
			break;
		}

		i++;
	}

	return indices;
}

std::vector<const char*> vulkan_application::get_required_extensions() const
{
	//obtain the number of extensions required
	uint32_t sdl_extension_count = 0;
	SDL_Vulkan_GetInstanceExtensions(sdl_window_, &sdl_extension_count, nullptr);

	//obtain the extensions required
	const auto sdl_extensions = new const char*[sdl_extension_count];
	SDL_Vulkan_GetInstanceExtensions(sdl_window_, &sdl_extension_count, sdl_extensions);
	std::vector<const char*> extensions(sdl_extensions, sdl_extensions + sdl_extension_count);

	return extensions;
}

bool vulkan_application::check_validation_layer_support()
{
	//get the list of layers supported
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	//get the layers supported
	std::vector<VkLayerProperties> vk_layer_propertieses(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, vk_layer_propertieses.data());

	//loop through each supported layer and check that our requested layers are supported
	for (auto layer_name : validation_layers)
	{
		auto layer_found = false;

		for (const auto& layer_properties : vk_layer_propertieses)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
		{
			return false;
		}
	}

	return true;
}

std::vector<char> vulkan_application::read_file(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	const auto file_size = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);

	file.close();

	return buffer;
}
