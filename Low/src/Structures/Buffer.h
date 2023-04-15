#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Low
{
	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(VkDevice device, VkPhysicalDevice physDevice, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProps);

		~Buffer();
		
		inline VkBuffer& Handle() { return m_Buffer; }
		inline VkDeviceMemory& Memory() { return m_Memory; }

		inline size_t Size() { return m_Size; }

	private:
		VkBuffer m_Buffer;
		VkDevice m_Device;
		VkDeviceMemory m_Memory;

		size_t m_Size;
	};
}