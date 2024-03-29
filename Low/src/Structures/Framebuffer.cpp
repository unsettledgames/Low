#include <Vulkan/VulkanCore.h>
#include <Hardware/Memory.h>
#include <Vulkan/Command/ImmediateCommands.h>
#include <Structures/Framebuffer.h>

namespace Low
{
	Framebuffer::Framebuffer(VkRenderPass renderPass, uint32_t width, uint32_t height, std::vector<FramebufferAttachmentSpecs>& specs,
		std::vector<VkImage> images) : m_Width(width), m_Height(height)
	{
		Init(renderPass, width, height, specs, images);
	}

	Framebuffer::~Framebuffer()
	{
		Cleanup();
	}

	void Framebuffer::Invalidate(VkRenderPass renderPass, uint32_t width, uint32_t height, std::vector<FramebufferAttachmentSpecs>& specs,
		std::vector<VkImage> images)
	{
		Cleanup();
		Init(renderPass, width, height, specs, images);
	}

	void Framebuffer::Init(VkRenderPass renderPass, uint32_t width, uint32_t height, std::vector<FramebufferAttachmentSpecs>& specs,
		std::vector<VkImage> images)
	{
		uint32_t attachmentIdx = 0;

		m_Attachments.resize(0);
		m_Width = width;
		m_Height = height;

		for (auto& config : specs)
		{
			FramebufferAttachment attachment;
			attachment.Specs = config;
			attachment.Image = images[attachmentIdx];

			// Create image if necessary
			if (!config.IsSwapchain)
			{
				VkImageCreateInfo texInfo = {};
				texInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				texInfo.imageType = VK_IMAGE_TYPE_2D;
				texInfo.extent.width = width;
				texInfo.extent.height = height;
				texInfo.extent.depth = 1;
				texInfo.mipLevels = 1;
				texInfo.arrayLayers = 1;
				texInfo.format = config.Format;
				texInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				texInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				texInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				texInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				texInfo.samples = (VkSampleCountFlagBits)(VK_SAMPLE_COUNT_1_BIT + config.SampleCount - 1);
				texInfo.flags = 0;

				if (vkCreateImage(VulkanCore::Device(), &texInfo, nullptr, &attachment.Image) != VK_SUCCESS)
					throw std::runtime_error("Couldn't create texture image");

				//ImmediateCommands::TransitionImageLayout(attachment.Image, texInfo.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

				VkMemoryRequirements memoryReqs;
				vkGetImageMemoryRequirements(VulkanCore::Device(), attachment.Image, &memoryReqs);

				VkMemoryAllocateInfo allocInfo = {};
				VkDeviceMemory memory = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memoryReqs.size;
				allocInfo.memoryTypeIndex = Memory::FindMemoryType(VulkanCore::PhysicalDevice(), memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				if (vkAllocateMemory(VulkanCore::Device(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
					throw std::runtime_error("Couldn't allocate texture memory");

				vkBindImageMemory(VulkanCore::Device(), attachment.Image, memory, 0);
				attachment.ImageMemory = memory;
			}

			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = attachment.Image;
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

			m_Attachments.push_back(attachment);
			attachmentIdx++;

			// TransitionImageToLayout(s_Data.DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		std::vector<VkImageView> attachments(m_Attachments.size());
		for (uint32_t i = 0; i < attachments.size(); i++)
			attachments[i] = m_Attachments[i].ImageView;
		VkFramebufferCreateInfo framebufferInfo{};

		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_Width;
		framebufferInfo.height = m_Height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VulkanCore::Device(), &framebufferInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create framebuffer");
	}

	void Framebuffer::Cleanup()
	{
		for (auto& attachment : m_Attachments)
		{
			if (!attachment.Specs.IsSwapchain)
				vkDestroyImage(VulkanCore::Device(), attachment.Image, nullptr);
			vkDestroyImageView(VulkanCore::Device(), attachment.ImageView, nullptr);
			vkFreeMemory(VulkanCore::Device(), attachment.ImageMemory, nullptr);
		}

		vkDestroyFramebuffer(VulkanCore::Device(), m_Handle, nullptr);
	}
}