#include <Vulkan/VulkanCore.h>
#include <Hardware/Memory.h>
#include <Structures/Framebuffer.h>

namespace Low
{
	Framebuffer::Framebuffer(FramebufferConfig& bufferConfig)
	{
		for (auto& config : bufferConfig.Attachments)
		{
			bool isSwapchain = true;
			FramebufferAttachment attachment;

			// Create image if necessary
			if (config.Image == VK_NULL_HANDLE)
			{
				isSwapchain = false;
				VkImageCreateInfo texInfo = {};
				texInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				texInfo.imageType = VK_IMAGE_TYPE_2D;
				texInfo.extent.width = config.Width;
				texInfo.extent.height = config.Height;
				texInfo.extent.depth = 1;
				texInfo.mipLevels = 1;
				texInfo.arrayLayers = 1;
				texInfo.format = config.Format;
				texInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				texInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				texInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				texInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				texInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				texInfo.flags = 0;

				if (vkCreateImage(VulkanCore::Device(), &texInfo, nullptr, &config.Image) != VK_SUCCESS)
					throw std::runtime_error("Couldn't create texture image");

				VkMemoryRequirements memoryReqs;
				vkGetImageMemoryRequirements(VulkanCore::Device(), config.Image, &memoryReqs);

				VkMemoryAllocateInfo allocInfo = {};
				VkDeviceMemory memory = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memoryReqs.size;
				allocInfo.memoryTypeIndex = Memory::FindMemoryType(VulkanCore::PhysicalDevice(), memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				if (vkAllocateMemory(VulkanCore::Device(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
					throw std::runtime_error("Couldn't allocate texture memory");

				vkBindImageMemory(VulkanCore::Device(), config.Image, memory, 0);
				attachment.ImageMemory = memory;
			}
			attachment.Image = config.Image;

			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = config.Image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = config.Format;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			if (config.Type == AttachmentType::Color)		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			else if (config.Type == AttachmentType::Depth)	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(VulkanCore::Device(), &createInfo, nullptr, &attachment.ImageView) != VK_SUCCESS)
				throw std::runtime_error("Couldn't create image view");

			VkAttachmentDescription description;
			VkAttachmentReference reference;

			description.format = config.Format;
			description.samples = (VkSampleCountFlagBits)(VK_SAMPLE_COUNT_1_BIT + config.SampleCount - 1);
			description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			if (config.Type == AttachmentType::Color)
			{
				description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				if (isSwapchain)
					description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				else
					description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (config.Type == AttachmentType::Depth)
			{
				description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}
		}
	}
}