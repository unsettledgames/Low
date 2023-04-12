#include <Resources/Texture.h>
#include <Structures/Buffer.h>
#include <Hardware/Memory.h>

#include <stb_image.h>

#include <stdexcept>

namespace Low
{
	Texture::Texture(const std::string& path, VkDevice device, VkPhysicalDevice physDevice) : m_Path(path), m_Device(device)
	{
		int width, height, nChannels;
		stbi_uc* pixels = stbi_load("../../Assets/Textures/texture.jpg", &width, &height, &nChannels, STBI_rgb_alpha);
		VkDeviceSize size = width * height * 4;

		// Read image and store data into buffers
		Buffer stagingBuffer(m_Device, physDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VkDeviceMemory stagingMemory;

		void* data;
		vkMapMemory(m_Device, stagingMemory, 0, size, 0, &data);
		memcpy(data, pixels, size);
		vkUnmapMemory(m_Device, stagingMemory);

		stbi_image_free(pixels);

		// Create texture image
		VkImage textureImage = {};
		VkDeviceMemory textureMemory = {};

		VkImageCreateInfo texInfo = {};
		texInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texInfo.imageType = VK_IMAGE_TYPE_2D;
		texInfo.extent.width = width;
		texInfo.extent.height = height;
		texInfo.extent.depth = 1;
		texInfo.mipLevels = 1;
		texInfo.arrayLayers = 1;
		texInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		texInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		texInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		texInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		texInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		texInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		texInfo.flags = 0;

		if (vkCreateImage(m_Device, &texInfo, nullptr, &textureImage) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create texture image");

		VkMemoryRequirements memoryReqs;
		vkGetImageMemoryRequirements(m_Device, textureImage, &memoryReqs);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryReqs.size;
		allocInfo.memoryTypeIndex = Memory::FindMemoryType(physDevice, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &textureMemory) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate texture memory");

		vkBindImageMemory(m_Device, textureImage, textureMemory, 0);
	}
}