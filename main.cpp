//Before including Vulkan, we need to tell it the platform that we are using
//this would be best done during the build process (e.g. cmake) however for
//this example I am defining it here
#define VK_USE_PLATFORM_WIN32_KHR
//Include the SDL2 API which will be used for window creation
#include <SDL.h>
#include <SDL_vulkan.h>
//Include the C Vulkan bindings
#include <vulkan/vulkan.h>
//Include the C++ Vulkan bindings
#include <vulkan/vulkan.hpp>

/*
 * Struct to define the layout of a swapchain buffer
 */

struct VkSwapchainBuffer {
    vk::Image image;
    std::array<vk::ImageView, 2> views;
    vk::Framebuffer buffer;
};

/*
 * Since Vulkan doesn't really keep track of state or provide
 * a "context" as such, we will need to keep track of various
 * variables and instances, this will be done in the following struct.
 */

struct VkContext {
    uint32_t window_width = 1280; //The width of the window
    uint32_t window_height = 720; //The height of the window
    SDL_Window* window; //A C pointer to the SDL window
    vk::UniqueInstance vulkanInstance; //A unique pointer to the Vulkan Instance
    VkSurfaceKHR vulkanCSurface; //A C instance of the surface
    vk::SurfaceKHR vulkanSurface; //c++ instance of the surface
    std::vector<vk::PhysicalDevice> vulkanPhysicalDevices;
    vk::PhysicalDevice vulkanPhysicalDevice;
    vk::Device vulkanGPU;
    vk::Extent2D vulkanSurfaceSize;
    uint32_t vulkanGraphicsFamilyIndex;
    vk::Queue graphicsQueue;
    vk::Format vulkanSurfaceColorFormat;
    vk::ColorSpaceKHR vulkanSurfaceColorSpace;
    vk::Format vulkanSurfaceDepthFormat;
    vk::RenderPass vulkanRenderPass;
    vk::SwapchainKHR vulkanSwapChain;
    std::vector<vk::Image> vulkanSwapChainImages;
    vk::Image vulkanDepthImage;
    vk::ImageView vulkanDepthImageView;
    std::vector<VkSwapchainBuffer> vulkanSwapChainBuffers;
    vk::Semaphore vulkanPresentCompletedSemaphore;
    vk::Semaphore vulkanRenderCompletedSemaphore;
    std::vector<vk::Fence> vulkanWaitFences;
    vk::CommandPool vulkanCommandPool;
    std::vector<vk::CommandBuffer> vulkanCommandBuffers;
    vk::DescriptorPool vulkanDescriptorPool;
};

//This is the instance of this context
VkContext myContext;

int main(int argc, char* argv[]) {

    /*
     * First, we must inform Vulkan of our application name,
     * the engine we are using, and the version of Vulkan that
     * we wish to use. The GPU driver uses these application/engine details
     * for optimisation, for example if the engine was Unreal then
     * certain features would be enabled or disabled by the driver.
     */
    vk::ApplicationInfo applicationInfo = vk::ApplicationInfo()
            .setPApplicationName("Vulkan Example")
            .setPEngineName("My Engine")
            .setApplicationVersion(1)
            .setEngineVersion(1)
            .setApiVersion(VK_MAKE_VERSION(1, 0, 0));

    /*
     * We are then required to define the instance structure. This is
     * where you pass the application information and decide which
     * extensions and validation layers will be enabled. Best practice
     * would be to check that the extensions and layers are supported on
     * this device. Howver, these are generally supported on desktop GPUs
     * so that step will be omitted.
     */
    std::vector<const char*> vulkan_extensions = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME, //Enable debugging
        VK_KHR_SURFACE_EXTENSION_NAME, //Enable surface support (for window rendering)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME //Enable windows surfaces
    };

    std::vector<const char*> vulkan_validation_layers = {
        "VK_LAYER_LUNARG_standard_validation", //Enable the standard debugging/validation layer
        "VK_LAYER_LUNARG_api_dump" //Output every vulkan api call to the console
    };

    vk::InstanceCreateInfo instanceCreateInfo = vk::InstanceCreateInfo()
            .setPApplicationInfo(&applicationInfo)
            .setEnabledExtensionCount(vulkan_extensions.size())
            .setPpEnabledExtensionNames(vulkan_extensions.data())
            .setEnabledLayerCount(vulkan_validation_layers.size())
            .setPpEnabledLayerNames(vulkan_validation_layers.data());

    //Now we create the Vulkan Instance
    myContext.vulkanInstance = vk::createInstanceUnique(instanceCreateInfo);

    /*
     * We will now initilize SDL2, and create a window. We can then use this
     * to create a surface to render to, and determine which device we will use to
     * render to this surface
     */
    SDL_Init(SDL_INIT_VIDEO);
    myContext.window = SDL_CreateWindow("Vulkan Example",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      myContext.window_width,
                                      myContext.window_height,
                                      SDL_WINDOW_VULKAN);

    SDL_Vulkan_CreateSurface(myContext.window, myContext.vulkanInstance.get(), &myContext.vulkanCSurface);
    //Because SDL is a C API, we need to copy the surface for use with C++
    myContext.vulkanSurface = vk::SurfaceKHR(myContext.vulkanCSurface);

    /*
     * We now need to create a logical device from a physical device (GPU)
     * to render with
     */
    myContext.vulkanPhysicalDevices = myContext.vulkanInstance->enumeratePhysicalDevices();
    myContext.vulkanPhysicalDevice = myContext.vulkanPhysicalDevices[0];

    //Specify which GPU extensions / layers we wish to enable

    std::vector<const char*> device_extensions =
    {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_DEBUG_MARKER_EXTENSION_NAME
    };

    std::vector<const char*> device_layers =
    {
      "VK_LAYER_LUNARG_standard_validation"
    };

    //We now need to find a queue on the GPU which supports graphics
    auto formatProperties =  myContext.vulkanPhysicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
    auto gpuFeatures = myContext.vulkanPhysicalDevice.getFeatures();
    auto gpuQueueProps = myContext.vulkanPhysicalDevice.getQueueFamilyProperties();

    float priority = 0.0;
    auto queueCreateInfos = std::vector<vk::DeviceQueueCreateInfo>();

    for (auto& queuefamily : gpuQueueProps)
    {
      if (queuefamily.queueFlags & vk::QueueFlagBits::eGraphics) {
        // Create a single graphics queue.
        queueCreateInfos.push_back(
          vk::DeviceQueueCreateInfo(
            vk::DeviceQueueCreateFlags(),
            myContext.vulkanGraphicsFamilyIndex,
            1,
            &priority
          )
        );
        break;
      }

      myContext.vulkanGraphicsFamilyIndex++;

    }

    //Create the device
    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo()
            .setQueueCreateInfoCount(queueCreateInfos.size())
            .setPQueueCreateInfos(queueCreateInfos.data())
            .setEnabledLayerCount(device_layers.size())
            .setPpEnabledLayerNames(device_layers.data())
            .setEnabledExtensionCount(device_extensions.size())
            .setPpEnabledExtensionNames(device_extensions.data())
            .setPEnabledFeatures(&gpuFeatures);

    myContext.vulkanGPU = myContext.vulkanPhysicalDevice.createDevice(deviceCreateInfo);

    //Retrieve the graphics queue
    myContext.graphicsQueue = myContext.vulkanGPU.getQueue(myContext.vulkanGraphicsFamilyIndex, 0);

    /*
     * We now need to determine what color formats the GPU spports, these will
     * determine what we can display and what buffers we can allocate.
     */
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = myContext.vulkanPhysicalDevice.getSurfaceFormatsKHR(myContext.vulkanSurface);

    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
      myContext.vulkanSurfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
    else
      myContext.vulkanSurfaceColorFormat = surfaceFormats[0].format;

    myContext.vulkanSurfaceColorSpace = surfaceFormats[0].colorSpace;

    vk::FormatProperties formatProps = myContext.vulkanPhysicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);

    //We now determine what depth format the GPU supports
    std::vector<vk::Format> depthFormats = {
      vk::Format::eD32SfloatS8Uint, //This is the best format, we want this one if possible
      vk::Format::eD32Sfloat,
      vk::Format::eD24UnormS8Uint,
      vk::Format::eD16UnormS8Uint,
      vk::Format::eD16Unorm
    };

    for(auto& format : depthFormats) {
        auto depthFormatProperties = myContext.vulkanPhysicalDevice.getFormatProperties(format);
        if(depthFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            myContext.vulkanSurfaceDepthFormat = format;
            break;
        }
    }

    /*
     * We now need to create a render pass, this describes the "postprocessing"
     * system. This is described as a list of subpasses. For example a color
     * buffer and a depth buffer will be required to show anything.
     */

    //Create a list of what "attachments" will be in the output buffer
    std::vector<vk::AttachmentDescription> attachmentDescriptions =
    {
        //Describe the color buffer
      vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(),
        myContext.vulkanSurfaceColorFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
      ),
        //Describe the depth buffer
      vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(),
        myContext.vulkanSurfaceDepthFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
      )
    };

    //Create a list of what attachments act on colors
    std::vector<vk::AttachmentReference> colorReferences =
    {
      vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)
    };

    //Create a list of what attachments act on depth
    std::vector<vk::AttachmentReference> depthReferences = {
      vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
    };

    //Create a subpass that targets the color
    std::vector<vk::SubpassDescription> subpasses =
    {
      vk::SubpassDescription(
        vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        colorReferences.size(),
        colorReferences.data(),
        nullptr,
        depthReferences.data(),
        0,
        nullptr
      )
    };

    //Create a list of what subpassses can use what other subpasses
    std::vector<vk::SubpassDependency> dependencies =
    {
      vk::SubpassDependency(
        ~0U,
        0,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eMemoryRead,
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlagBits::eByRegion
      ),
      vk::SubpassDependency(
        0,
        ~0U,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eMemoryRead,
        vk::DependencyFlagBits::eByRegion
      )
    };

    //Now create the render pass using what we defined
    vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
            .setAttachmentCount(attachmentDescriptions.size())
            .setPAttachments(attachmentDescriptions.data())
            .setSubpassCount(subpasses.size())
            .setPSubpasses(subpasses.data())
            .setDependencyCount(dependencies.size())
            .setPDependencies(dependencies.data());
    myContext.vulkanRenderPass = myContext.vulkanGPU.createRenderPass(renderPassInfo);

    /*
     * We now need to define a SwapChain, this is a structure which manages
     * the allocation of framebuffers which are cycled through by the application.
     * In OpenGL/SDL for example this is what is done when RenderPresent is called
     * or SwapBuffers. This is how V-Sync is enabled through double or triple
     * buffering. For double buffering there are 2 framebuffers, one is the current
     * frame and the other is the next frame. These are swapped to prevent tearing.
     */
    vk::SurfaceCapabilitiesKHR surfaceCaps = myContext.vulkanPhysicalDevice.getSurfaceCapabilitiesKHR(myContext.vulkanSurface);
    std::vector<vk::PresentModeKHR> surfacePresentModes = myContext.vulkanPhysicalDevice.getSurfacePresentModesKHR(myContext.vulkanSurface);

    //Check the surface width / height
    if (!(surfaceCaps.currentExtent.width == -1 || surfaceCaps.currentExtent.height == -1)) {
      myContext.vulkanSurfaceSize = surfaceCaps.currentExtent;
    }

    //Check which presentation mode is supported, the preferred mode
    //is "mailbox" however if this is not available then default to
    //immediate presentation. However this can result in "tearing"
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

    for (auto& pm : surfacePresentModes) {
      if (pm == vk::PresentModeKHR::eMailbox) {
        presentMode = vk::PresentModeKHR::eMailbox;
        break;
      }
    }

    std::vector<uint32_t> queueFamilyIdices;
    queueFamilyIdices.push_back(myContext.vulkanGraphicsFamilyIndex);

    //Now create the swapchain
    vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
            .setSurface(myContext.vulkanSurface)
            .setMinImageCount(3)
            .setImageFormat(myContext.vulkanSurfaceColorFormat)
            .setImageColorSpace(myContext.vulkanSurfaceColorSpace)
            .setImageExtent(myContext.vulkanSurfaceSize)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setQueueFamilyIndexCount(queueFamilyIdices.size())
            .setPQueueFamilyIndices(queueFamilyIdices.data())
            .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(presentMode);

    myContext.vulkanSwapChain = myContext.vulkanGPU.createSwapchainKHR(swapchainCreateInfo);
    myContext.vulkanSwapChainImages = myContext.vulkanGPU.getSwapchainImagesKHR(myContext.vulkanSwapChain);

    /*
     * Now we setup the framebuffer. A framebuffer is a container of image views.
     * A view is a handle to a resource on a GPU such as an image or buffer.
     */

    //Create the Depth Image data
    vk::ImageCreateInfo depthImageInfo = vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(myContext.vulkanSurfaceDepthFormat)
            .setExtent(vk::Extent3D(myContext.vulkanSurfaceSize.width, myContext.vulkanSurfaceSize.height, 1))
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setQueueFamilyIndexCount(queueFamilyIdices.size())
            .setPQueueFamilyIndices(queueFamilyIdices.data())
            .setArrayLayers(1)
            .setMipLevels(1);

    myContext.vulkanDepthImage = myContext.vulkanGPU.createImage(depthImageInfo);

    //Search through the GPU's memory properties to see if this image
    //can be device local
    auto depthMemoryReq = myContext.vulkanGPU.getImageMemoryRequirements(myContext.vulkanDepthImage);
    uint32_t typeBits = depthMemoryReq.memoryTypeBits;
    uint32_t depthMemoryTypeIndex;

    vk::PhysicalDeviceMemoryProperties gpuMemoryProps = myContext.vulkanPhysicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < gpuMemoryProps.memoryTypeCount; i++)
    {
      if ((typeBits & 1) == 1)
      {
        if ((gpuMemoryProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
          depthMemoryTypeIndex = i;
          break;
        }
      }
      typeBits >>= 1;
    }

    //Allocate the memory space required for this image on the GPU
    auto depthMemory = myContext.vulkanGPU.allocateMemory(
      vk::MemoryAllocateInfo(depthMemoryReq.size, depthMemoryTypeIndex)
    );

    //Bind that memory space to the image
    myContext.vulkanGPU.bindImageMemory(myContext.vulkanDepthImage, depthMemory, 0);

    //Now create an image view for this depth image
    vk::ImageViewCreateInfo depthImageViewInfo = vk::ImageViewCreateInfo()
            .setImage(myContext.vulkanDepthImage)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(myContext.vulkanSurfaceDepthFormat)
            .setComponents(vk::ComponentMapping())
            .setSubresourceRange(vk::ImageSubresourceRange(
                                     vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                                     0, 1, 0, 1));

    myContext.vulkanDepthImageView = myContext.vulkanGPU.createImageView(depthImageViewInfo);

   //Create the swapchain
   myContext.vulkanSwapChainBuffers.resize(myContext.vulkanSwapChainImages.size());

   for(int i = 0; i < myContext.vulkanSwapChainImages.size(); i++) {
       myContext.vulkanSwapChainBuffers[i].image = myContext.vulkanSwapChainImages[i];

       //Color Buffer
      myContext.vulkanSwapChainBuffers[i].views[0] =
           myContext.vulkanGPU.createImageView(
             vk::ImageViewCreateInfo(
               vk::ImageViewCreateFlags(),
               myContext.vulkanSwapChainImages[i],
               vk::ImageViewType::e1D,
              myContext.vulkanSurfaceColorFormat,
               vk::ComponentMapping(),
               vk::ImageSubresourceRange(
                 vk::ImageAspectFlagBits::eColor,
                 0,
                 1,
                 0,
                 1
               )
             )
           );

       //Depth Buffer
      myContext.vulkanSwapChainBuffers[i].views[1] = myContext.vulkanDepthImageView;

        myContext.vulkanSwapChainBuffers[i].buffer = myContext.vulkanGPU.createFramebuffer(
          vk::FramebufferCreateInfo(
            vk::FramebufferCreateFlags(),
            myContext.vulkanRenderPass,
            myContext.vulkanSwapChainBuffers[i].views.size(),
            myContext.vulkanSwapChainBuffers[i].views.data(),
            myContext.vulkanSurfaceSize.width,
            myContext.vulkanSurfaceSize.height,
            1
          )
        );
   }

   /*
    * We now need to define some "seamphores". Vulkan is designed for concurrency,
    * we therefore need to "synchronize" things with the GPU.
    * A semaphore coordinates operations within the graphics queue and ensures
    * commands are ran in the correct order. The syncrhonisation also ensures that
    * GPU memory is not accessed incorrectly.
    */
   myContext.vulkanPresentCompletedSemaphore = myContext.vulkanGPU.createSemaphore(vk::SemaphoreCreateInfo());
   myContext.vulkanRenderCompletedSemaphore = myContext.vulkanGPU.createSemaphore(vk::SemaphoreCreateInfo());

   //Create a "fence" which is used for command buffer completion
    myContext.vulkanWaitFences.resize(myContext.vulkanSwapChainBuffers.size());
    for(int i = 0; i < myContext.vulkanWaitFences.size(); i++) {
        myContext.vulkanWaitFences[i] = myContext.vulkanGPU.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

    /*
     * We now need to create a command pool. This is a means of allocating command buffers.
     * A command buffer contains a list of recorded actions to perform on the GPU. Any number
     * of command buffers can be made from a command pool, however we are responsible for managing these
     */

    vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(myContext.vulkanGraphicsFamilyIndex);

    myContext.vulkanCommandPool = myContext.vulkanGPU.createCommandPool(commandPoolInfo);

    //Allocate 1 command buffer for each image within the swap chain
    myContext.vulkanCommandBuffers = myContext.vulkanGPU.allocateCommandBuffers(
                vk::CommandBufferAllocateInfo(
                myContext.vulkanCommandPool,
                vk::CommandBufferLevel::ePrimary,
                myContext.vulkanSwapChainBuffers.size()
              )
            );

    /*
     * A descriptor pool is now required in order to allocate Descriptor Sets. These
     * are data structures that contain implementation specific descriptions of
     * resources, for example a Uniform Buffer for use with shaders
     */
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
        vk::DescriptorPoolSize(
        vk::DescriptorType::eUniformBuffer,
        1
        )
    };

    vk::DescriptorPoolCreateInfo descriptorPoolInfo = vk::DescriptorPoolCreateInfo()
            .setMaxSets(1)
            .setPoolSizeCount(descriptorPoolSizes.size())
            .setPPoolSizes(descriptorPoolSizes.data());

    myContext.vulkanDescriptorPool = myContext.vulkanGPU.createDescriptorPool(descriptorPoolInfo);

    /*
     * A descriptor set stores the resources bound in a shader. They connect the
     * binding points of a shader with the buffers and images used for these things.
     * A descriptor set needs a layout, and a binding.
     */

    //Binding 0 Uniform Buffer
    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings =
    {
      vk::DescriptorSetLayoutBinding(
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex,
        nullptr
      )
    };

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
      myContext.vulkanGPU.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo(
          vk::DescriptorSetLayoutCreateFlags(),
          descriptorSetLayoutBindings.size(),
          descriptorSetLayoutBindings.data()
      )
      )
    };

    auto descriptorSets = myContext.vulkanGPU.allocateDescriptorSets(
      vk::DescriptorSetAllocateInfo(
        myContext.vulkanDescriptorPool,
        descriptorSetLayouts.size(),
        descriptorSetLayouts.data()
      )
    );

    /*
     * We now need to define the graphics pipeline.
     */

    bool quit = false;
    SDL_Event e;

    while(!quit) {
        while( SDL_PollEvent( &e ) != 0 )
        {
            if( e.type == SDL_QUIT )
            {
                quit = true;
            }
        }
    }

    SDL_DestroyWindow(myContext.window);
    SDL_Quit();

    return 0;
}
