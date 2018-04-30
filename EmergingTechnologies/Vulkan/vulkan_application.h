/**
* \class vulkan_application
*
* \brief Provide an example of how to use Vulkan 
*
* This is a detailed example of how you can draw a rotating object
* using the Vulkan API. This example uses staging buffers to upload
* data to the GPU, supports uniform buffers and is heavily commented.
* It can be used as a base for a vulkan application. This example has
* only been tested on Windows, however should work on Linux with minor
* changes. The SDL2 library is used for window and surface creation.
*
* \author Alixander Roden
*		  The University Of Suffolk
*		  Ipswich, United Kingdom
*
* \date 2018/04/30 05:01:00
*
* Contact: S190762@uos.ac.uk
*
* Last Updated on: 30th April 2018
*
*/


#ifndef TRIANGLE_H
#define TRIANGLE_H

//Include Vulkan, tell vulkan this is the Win32 platform
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

//Include SDL2 and the SDL Vulkan library
#include <SDL.h>

//Include the GLM math library
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <chrono>
#include <vector>
#include <array>

/**
* \brief Define which validation layers to load
*/
const std::vector<const char*> validation_layers = {
	"VK_LAYER_LUNARG_standard_validation" //Enable "standard" vulkan validation
};

/**
* \brief Define the device extensions to load
*/
const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, //Enable support for a swapchain
};

/**
* \brief A structure that holds the graphics family index and present family index
*/
struct queue_family_indices
{
public:
	//set to -1 because there can be a queue family with an index of 0
	int graphics_family = -1;
	int present_family = -1;

	bool is_complete() const
	{
		return graphics_family >= 0 && present_family >= 0;
	}
};

/**
* \brief Hold the surface capabilities, surface formats and present modes. Used
* to determine if a swap chain is supported
*/
struct swap_chain_support_details
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

/**
* \brief A structure that defines the layout of a Vertex and it's associated data
*/
struct vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	/**
	* \brief Define the vertex input
	* \return A vulkan input binding description
	*/
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription binding_description = {};
		binding_description.binding = 0;
		binding_description.stride = sizeof(vertex); //the memory size of this struct
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //tell vulkan this is a vertex input

		return binding_description;
	}

	/**
	* \brief Obtain a description of the attributes for a vertex
	* \return std array containing 2 attribute descriptions
	*/
	static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions()
	{
		//initialise the array
		std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = {};

		//2 float array at location 0 with an offset of the first element in the struct
		//glm::vec2
		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT; //2 floats
		attribute_descriptions[0].offset = offsetof(vertex, pos);

		//3 float array at location 1 with an offset of the second element in the struct
		//glm::vec3
		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; //3 floats
		attribute_descriptions[1].offset = offsetof(vertex, color);

		return attribute_descriptions;
	}
};

/**
* \brief A structure to hold the data to be sent to the shader
*/
struct uniform_buffer_object
{
	glm::mat4 model; //Model matrix (Model Transform)
	glm::mat4 view; //View Matrix (Camera)
	glm::mat4 proj; //Proj Matrix (Perspective)
};

/**
* \brief The vertices to upload to the GPU, the format is
* {X, Y}{R, G, B}
*/
const std::vector<vertex> vertices = {
	{{-0.5F, -0.5F},{1.0F, 0.0F, 0.0F}},
	{{0.5F, -0.5F},{0.0F, 1.0F, 0.0F}},
	{{0.5F, 0.5F},{0.0F, 0.0F, 1.0F}},
	{{-0.5F, 0.5F},{1.0F, 1.0F, 1.0F}}
};

/**
* \brief The indices to upload to the GPU
*/
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

/**
* \brief The main application class
*/
class vulkan_application
{
public:
	/**
	* \brief The method to be called to run the application
	*/
	void run();
protected:
	/**
	* \brief Define the height and width of the window
	*/
	int width_ = 800;
	int height_ = 600;
private:
	SDL_Window* sdl_window_; // A pointer to the SDL window

	VkInstance vulkan_instance_; //Vulkan Instance
	VkDebugReportCallbackEXT debug_callback_;
	VkSurfaceKHR vulkan_surface_;

	//Device
	VkPhysicalDevice physical_device_ = nullptr;
	VkDevice logical_device_;

	//Device Queues
	VkQueue graphics_queue_;
	VkQueue present_queue_;

	//Swap Chain
	VkSwapchainKHR swap_chain_;
	std::vector<VkImage> swap_chain_images_;
	VkFormat swap_chain_image_format_;
	VkExtent2D swap_chain_extent_;
	std::vector<VkImageView> swap_chain_image_views_;
	std::vector<VkFramebuffer> swap_chain_framebuffers_;

	//Graphics Pipeline
	VkRenderPass render_pass_;
	VkDescriptorSetLayout descriptor_set_layout_;
	VkPipelineLayout pipeline_layout_;
	VkPipeline graphics_pipeline_;

	//Commands
	VkCommandPool command_pool_;
	std::vector<VkCommandBuffer> command_buffers_;

	//Buffers
	VkBuffer vertex_buffer_;
	VkDeviceMemory vertex_buffer_memory_;
	VkBuffer index_buffer_;
	VkDeviceMemory index_buffer_memory_;
	VkBuffer uniform_buffer_;
	VkDeviceMemory uniform_buffer_memory_;

	//Descriptor Sets
	VkDescriptorPool descriptor_pool_;
	VkDescriptorSet descriptor_set_;

	//Synchronization
	VkSemaphore image_available_semaphore_;
	VkSemaphore render_finished_semaphore_;


	/**
	* \brief Initialize the window using the SDL library
	*/
	void init_window();


	/**
	* \brief Initialize the various Vulkan elements
	*/
	void init_vulkan();

	/**
	* \brief The main loop of the application
	*/
	void main_loop();

	/**
	* \brief Destroy the various vulkan elements associated with the swap chain
	*/
	void cleanup_swap_chain();


	/**
	* \brief Called when the program is quitting, destroys all vulkan elements and
	* exits SDL
	*/
	void cleanup();


	/**
	* \brief Called on window resizing,
	*/
	void recreate_swap_chain();


	/**
	* \brief Create a Vulkan Instance
	*/
	void create_instance();

	/**
	* \brief Obtain the Surface to render to
	*/
	void create_surface();

	/**
	* \brief Obtain the physical device we wish to use
	*/
	void pick_physical_device();

	/**
	* \brief Create a logical device from a physical device
	*/
	void create_logical_device();


	/**
	* \brief Create the swap chain
	*/
	void create_swap_chain();


	/**
	* \brief Create views for the associated images
	*/
	void create_image_views();

	/**
	* \brief Create a render pass
	*/
	void create_render_pass();

	/**
	* \brief Define the descriptor set layout, which is used to send the uniform buffer to the GPU
	*/
	void create_descriptor_set_layout();

	/**
	* \brief Create a graphics pipeline which will be used to render.
	* The graphics pipeline we will create will be similar to the pipeline in OpenGL
	*/
	void create_graphics_pipeline();

	/**
	* \brief Obtain the framebuffers from the device, used for drawing
	*/
	void create_framebuffers();

	/**
	* \brief Obtain the command pool, which will be used to contain commands
	*/
	void create_command_pool();

	/**
	* \brief Create the vertex buffer, this is where the data of the vertices to draw will be held in the GPU memory
	*/
	void create_vertex_buffer();

	/**
	* \brief Create the index buffer, this is where the data of the indices to use when drawing will be held in the GPU memory
	*/
	void create_index_buffer();

	/**
	* \brief Create the uniform buffer, this is where the data will be held in GPU memory to be used by the vertex shader
	* NOTE: a staging buffer is not used here because this will be constantly updated by the CPU
	*/
	void create_uniform_buffer();

	/**
	* \brief Create a descriptor pool, which will be used by the uniform buffer.
	*/
	void create_descriptor_pool();

	/**
	* \brief Define the uniform buffer object descriptor set to send to the GPU
	*/
	void create_descriptor_set();

	/**
	* \brief Create a buffer on the GPU
	* \param size the size of the buffer data
	* \param usage the usage of the buffer
	* \param properties the memroy properties
	* \param buffer the buffer
	* \param buffer_memory the buffer's memory
	*/
	void create_buffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties,
	                   VkBuffer& buffer, VkDeviceMemory& buffer_memory) const;

	/**
	* \brief Copy from one GPU buffer to another
	* \param src_buffer the buffer to copy from
	* \param dst_buffer the buffer to copy to
	* \param size the size of the buffer data
	*/
	void copy_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size) const;


	/**
	* \brief Obtain the memory type that the GPU supports
	* \param type_filter what the memory type must support
	* \param properties the memory property flags
	* \return the memory type
	*/
	uint32_t find_memory_type(const uint32_t type_filter, const VkMemoryPropertyFlags properties) const;

	/**
	* \brief Create the command buffers which will then be used for submitting rendering commands
	*/
	void create_command_buffers();

	/**
	* \brief Create synchronization semaphores, one will be used to signal when an image is available for rendering to
	* the other will be used to signal when that rendering is finished
	*/
	void create_semaphores();

	/**
	* \brief This is where the data is sent to the uniform buffer for use in the vertex shader
	*/
	void update_uniform_buffer() const;

	/**
	* \brief Called on each update, to draw to the surface
	*/
	void draw_frame();

	/**
	* \brief Create a shader module from SPIR-V shader code
	* \param code the data of the shader
	* \return a shader module
	*/
	VkShaderModule create_shader_module(const std::vector<char>& code) const;

	/**
	* \brief Obtain the best surface format for the swapchain
	* \param available_formats a list of formats the surface supports
	* \return the best surface format to use
	*/
	static VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);

	/**
	* \brief Obtain the best present mode for the swapchain
	* \param available_present_modes a list of present modes the surface supports
	* \return the best present mode to use
	*/
	static VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> available_present_modes);

	/**
	* \brief Obtain the size of the window for use in the swapchain
	* \param capabilities a list of surface capabilites
	* \return the size of the swapchain extent (window width and height)
	*/
	VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

	/**
	* \brief Obtain various data about the device for use when creating a swapchain
	* \param device the device that we wish to use for the swap chain
	* \return a struct containg data about the device's swapchain capabilities
	*/
	swap_chain_support_details query_swap_chain_support(const VkPhysicalDevice device) const;

	/**
	* \brief Check if the device is suitable for use
	* \param device the device that we wish to check is suitable
	* \return true if the device is suitable, false if not
	*/
	bool is_device_suitable(const VkPhysicalDevice device) const;

	/**
	* \brief Check if the devie supports the extensions we need
	* \param device the device to check
	* \return true/false
	*/
	static bool check_device_extension_support(const VkPhysicalDevice device);

	/**
	* \brief Find the graphics queue and present queue id's for the device
	* \param device the device to search
	* \return a structure containg the queue id's
	*/
	queue_family_indices find_queue_families(const VkPhysicalDevice device) const;

	/**
	* \brief Get the device extensions that are required by SDL
	* \return a list of the extensions
	*/
	std::vector<const char*> get_required_extensions() const;


	/**
	* \brief Check that our requested validation layers are supported by the device
	* \return true/false
	*/
	static bool check_validation_layer_support();

	/**
	* \brief Helper function to read a file from disk
	* \param filename the name of the file to read
	* \return a char vector containing the read data
	*/
	static std::vector<char> read_file(const std::string& filename);
};

#endif TRIANGLE_H
