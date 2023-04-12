#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace Low
{
	class Texture
	{
	public:
		Texture(const std::string& path, VkDevice device, VkPhysicalDevice physDevice);

		inline VkImage GetImage() { return m_Image; }
		inline VkDeviceMemory GetMemory() { return m_Memory; }

		inline uint32_t GetWidth() { return m_Width; }
		inline uint32_t GetHeight() { return m_Height; }
		inline uint32_t GetChannelCount() { return m_ChannelCount; }

	private:
		std::string m_Path;
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_ChannelCount;

		VkDevice m_Device;
		VkImage m_Image;
		VkDeviceMemory m_Memory;
	};
}