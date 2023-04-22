#include <Structures/Buffer.h>
#include <Hardware/Memory.h>

#include <Vulkan/VulkanCore.h>
#include <Vulkan/Command/OneTimeCommands.h>

#include <stdexcept>

namespace Low
{
	Buffer::Buffer(uint32_t size, BufferUsage usage) : m_Size(size)
	{
		Init(size, usage);
	}

	Buffer::Buffer(uint32_t size, void* data, BufferUsage usage) : m_Size(size)
	{
		Init(size, usage);

		void* tmpData;
		Buffer stagingBuffer(size, BufferUsage::TransferSrc);

		if (vkMapMemory(VulkanCore::Device(), stagingBuffer.Memory(), 0, size, 0, &tmpData) != VK_SUCCESS)
			std::cout << "Mapping failed" << std::endl;
		memcpy(tmpData, data, (size_t)size);
		vkUnmapMemory(VulkanCore::Device(), stagingBuffer.Memory());

		OneTimeCommands::CopyBuffer(m_Handle,  stagingBuffer.Handle(), size);
	}

	void Buffer::Init(uint32_t size, BufferUsage usage)
	{
		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		switch (usage)
		{
		case BufferUsage::TransferSrc:	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; break;
		case BufferUsage::TransferDst:	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT; break;
		case BufferUsage::Vertex:		createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
		case BufferUsage::Index:		createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
		case BufferUsage::Uniform:		createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
		default: break;
		}

		if (vkCreateBuffer(VulkanCore::Device(), &createInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create buffer");

		VkMemoryRequirements memRequirements = {};
		vkGetBufferMemoryRequirements(VulkanCore::Device(), m_Handle, &memRequirements);

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memRequirements.size;

		VkMemoryPropertyFlags memoryProps = 0;
		switch (usage)
		{
		case BufferUsage::TransferDst:
		case BufferUsage::TransferSrc:	memoryProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; break;
		case BufferUsage::Vertex:		memoryProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break;
		case BufferUsage::Index:		memoryProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break;
		case BufferUsage::Uniform:		memoryProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; break;
		default: break;
		}

		allocateInfo.memoryTypeIndex = Memory::FindMemoryType(VulkanCore::PhysicalDevice(), memRequirements.memoryTypeBits, memoryProps);

		if (vkAllocateMemory(VulkanCore::Device(), &allocateInfo, nullptr, &m_Memory) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate memory for the vertex buffer");
		if (vkBindBufferMemory(VulkanCore::Device(), m_Handle, m_Memory, 0) != VK_SUCCESS)
			throw std::runtime_error("Couldn't bind memory to buffer");
	}

	Buffer::~Buffer()
	{
		vkDestroyBuffer(VulkanCore::Device(), m_Handle, nullptr);
		vkFreeMemory(VulkanCore::Device(), m_Memory, nullptr);
	}
}