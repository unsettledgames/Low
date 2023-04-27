#include <Resources/Texture.h>
#include <Structures/Buffer.h>
#include <Hardware/Memory.h>

#include <Vulkan/VulkanCore.h>
#include <Vulkan/Command/OneTimeCommands.h>

#include <stb_image.h>

#include <stdexcept>

namespace Low
{
	Texture::Texture(const std::string& path, VkFormat format, VkImageTiling tiling) : 
		m_Path(path), m_Format(format), m_Tiling(tiling)
	{
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* pixels = stbi_load(path.c_str(), &m_Width, &m_Height, &m_ChannelCount, STBI_rgb_alpha);
		VkDeviceSize size = m_Width * m_Height * 4;

		// Read image and store data into buffers
		m_Buffer = CreateRef<Low::Buffer>(size, BufferUsage::TransferSrc);

		void* data;
		vkMapMemory(VulkanCore::Device(), m_Buffer->Memory(), 0, size, 0, &data);
		memcpy(data, pixels, size);
		vkUnmapMemory(VulkanCore::Device(), m_Buffer->Memory());

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

		if (vkCreateImage(VulkanCore::Device(), &texInfo, nullptr, &m_Image) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create texture image");

		VkMemoryRequirements memoryReqs;
		vkGetImageMemoryRequirements(VulkanCore::Device(), m_Image, &memoryReqs);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryReqs.size;
		allocInfo.memoryTypeIndex = Memory::FindMemoryType(VulkanCore::PhysicalDevice(), memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(VulkanCore::Device(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate texture memory");

		vkBindImageMemory(VulkanCore::Device(), m_Image, m_Memory, 0);

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

		if (vkCreateImageView(VulkanCore::Device(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create image view");

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = true;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(VulkanCore::PhysicalDevice(), &properties);

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(VulkanCore::Device(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create sampler");

		OneTimeCommands::TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		OneTimeCommands::CopyBufferToImage(m_Image, m_Buffer->Handle(), m_Width, m_Height);
		OneTimeCommands::TransitionImageLayout(m_Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	Texture::~Texture()
	{
		vkDestroyImageView(VulkanCore::Device(), m_ImageView, nullptr);
		vkDestroySampler(VulkanCore::Device(), m_Sampler, nullptr);
		vkDestroyImage(VulkanCore::Device(), m_Image, nullptr);
		vkFreeMemory(VulkanCore::Device(), m_Memory, nullptr);
	}
}