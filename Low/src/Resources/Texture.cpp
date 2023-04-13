#include <Resources/Texture.h>
#include <Structures/Buffer.h>
#include <Hardware/Memory.h>

#include <stb_image.h>

#include <stdexcept>

namespace Low
{
	Texture::Texture(const std::string& path, VkDevice device, VkPhysicalDevice physDevice, VkFormat format, VkImageTiling tiling) : 
		m_Path(path), m_Device(device), m_Format(format), m_Tiling(tiling)
	{
		stbi_uc* pixels = stbi_load("../../Assets/Textures/texture.jpg", &m_Width, &m_Height, &m_ChannelCount, STBI_rgb_alpha);
		VkDeviceSize size = m_Width * m_Height * 4;

		// Read image and store data into buffers
		m_Buffer = CreateRef<Low::Buffer>(m_Device, physDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(m_Device, m_Buffer->Memory(), 0, size, 0, &data);
		memcpy(data, pixels, size);
		vkUnmapMemory(m_Device, m_Buffer->Memory());

		stbi_image_free(pixels);

		// Create texture image
		m_Image = {};
		m_Memory = {};

		VkImageCreateInfo texInfo = {};
		texInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texInfo.imageType = VK_IMAGE_TYPE_2D;
		texInfo.extent.width = m_Width;
		texInfo.extent.height = m_Height;
		texInfo.extent.depth = 1;
		texInfo.mipLevels = 1;
		texInfo.arrayLayers = 1;
		texInfo.format = m_Format;
		texInfo.tiling = m_Tiling;
		texInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		texInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		texInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		texInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		texInfo.flags = 0;

		if (vkCreateImage(m_Device, &texInfo, nullptr, &m_Image) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create texture image");

		VkMemoryRequirements memoryReqs;
		vkGetImageMemoryRequirements(m_Device, m_Image, &memoryReqs);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryReqs.size;
		allocInfo.memoryTypeIndex = Memory::FindMemoryType(physDevice, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate texture memory");

		vkBindImageMemory(m_Device, m_Image, m_Memory, 0);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create image view");
	}

	Texture::~Texture()
	{
		vkDestroyImageView(m_Device, m_ImageView, nullptr);
		vkDestroySampler(m_Device, m_Sampler, nullptr);
		vkDestroyImage(m_Device, m_Image, nullptr);
		vkFreeMemory(m_Device, m_Memory, nullptr);
	}

	VkBuffer Texture::Buffer() { return m_Buffer->Handle(); }
}