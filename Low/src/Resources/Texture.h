#pragma once

namespace Low
{
	class Buffer;

	class Texture
	{
	public:
		Texture(const std::string& path, VkDevice device, VkPhysicalDevice physDevice, VkFormat format, VkImageTiling tiling);
		~Texture();

		inline VkImage Handle() { return m_Image; }
		inline VkDeviceMemory Memory() { return m_Memory; }
		inline VkSampler* Sampler() { return &m_Sampler; }
		inline VkImageView ImageView() { return m_ImageView; }
		VkBuffer Buffer();

		inline uint32_t GetWidth() { return m_Width; }
		inline uint32_t GetHeight() { return m_Height; }
		inline uint32_t GetChannelCount() { return m_ChannelCount; }

	private:
		std::string m_Path;
		int m_Width;
		int m_Height;
		int m_ChannelCount;

		VkDevice m_Device;
		VkImage m_Image;
		VkImageView m_ImageView;
		VkSampler m_Sampler;

		VkDeviceMemory m_Memory;
		Ref<Low::Buffer> m_Buffer;

		VkFormat m_Format;
		VkImageTiling m_Tiling;
	};
}