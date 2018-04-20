#pragma once
#include <vulkan/vulkan.h>
#include <map>
#include "VulkanDevice.h"

struct vulkan_buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDescriptorBufferInfo descriptor;
	const vulkan_device* vdevice;

	void create(const vulkan_device* device, 
		const VkDeviceSize size, 
		const VkBufferUsageFlags usage, 
		const VkMemoryPropertyFlags memory_properties)
	{
		vdevice = device;
		device->create_buffer_and_memory(
			size,
			usage,
			memory_properties,
			buffer,
			memory
		);
		descriptor.buffer = buffer;
		descriptor.offset = 0;
		descriptor.range = size;
	}

	void destroy() const
	{
		vkDestroyBuffer(vdevice->device, buffer, nullptr);
		vkFreeMemory(vdevice->device, memory, nullptr);
	}
};

struct geometry_buffer_offset
{
	std::map<vertex_attribute, VkDeviceSize> vertex_buffer_offsets;
};

struct geometry_buffer
{
	geometry_buffer_offset buffer_layout;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
};