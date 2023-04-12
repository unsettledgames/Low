#include <Structures/Buffer.h>
#include <Hardware/Memory.h>

#include <stdexcept>
#include <vulkan/vulkan.h>

namespace Low
{
	Buffer::Buffer(VkDevice device, VkPhysicalDevice physDevice, uint32_t size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryProps) : m_Device(device)
	{
		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &createInfo, nullptr, &m_Buffer) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create buffer");

		VkMemoryRequirements memRequirements = {};
		vkGetBufferMemoryRequirements(device, m_Buffer, &memRequirements);

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memRequirements.size;
		allocateInfo.memoryTypeIndex = Memory::FindMemoryType(physDevice, memRequirements.memoryTypeBits, memoryProps);

		if (vkAllocateMemory(device, &allocateInfo, nullptr, &m_Memory) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate memory for the vertex buffer");
		if (vkBindBufferMemory(device, m_Buffer, m_Memory, 0) != VK_SUCCESS)
			throw std::runtime_error("Couldn't bind memory to buffer");
	}

	Buffer::~Buffer()
	{
		vkDestroyBuffer(m_Device, m_Buffer, nullptr);
		vkFreeMemory(m_Device, m_Memory, nullptr);
	}
}