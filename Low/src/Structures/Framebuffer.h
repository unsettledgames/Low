#pragma once

namespace Low
{
	enum class AttachmentType {None = 0, Color, Depth};

	struct FramebufferAttachmentConfig
	{
		AttachmentType Type = AttachmentType::None;

		VkImage Image = VK_NULL_HANDLE;
		VkFormat Format = VK_FORMAT_UNDEFINED;

		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t SampleCount = 1;

		FramebufferAttachmentConfig() = default;
		// Swapchain attachment
		FramebufferAttachmentConfig(AttachmentType type, VkImage image, uint32_t width, uint32_t height, VkFormat format) :
			Type(type), Image(image), Width(width), Height(height), Format(format) {}
		// User defined attachment
		FramebufferAttachmentConfig(AttachmentType type, uint32_t width, uint32_t height, VkFormat format, uint32_t nSamples = 1) :
			Type(type), Format(format), Width(width), Height(height), SampleCount(nSamples) {}
	};

	struct FramebufferAttachmentSpecs
	{
		AttachmentType Type;
		uint32_t SampleCount;
		bool IsSwapchain;
		int Index;

		VkAttachmentDescription Description = {};
		VkAttachmentReference Reference = {};
		VkFormat Format;

		FramebufferAttachmentSpecs() = default;
		FramebufferAttachmentSpecs(AttachmentType type, VkFormat format, uint32_t samples, bool isSwapchain) :
			Type(type), SampleCount(samples), IsSwapchain(IsSwapchain), Format(format)
		{
			static int index = 0;

			VkAttachmentDescription description = {};
			description.format = Format;
			description.flags = 0;
			description.samples = (VkSampleCountFlagBits)(VK_SAMPLE_COUNT_1_BIT + SampleCount - 1);
			description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			if (Type == AttachmentType::Color)
			{
				description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				if (isSwapchain)
					description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				else
					description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			else if (Type == AttachmentType::Depth)
			{
				description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			VkAttachmentReference reference = {};
			reference.attachment = index;
			if (Type != AttachmentType::Depth)
				reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			else
				reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			index++;
		}
	};

	struct FramebufferAttachment
	{
		FramebufferAttachmentSpecs Specs;

		VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;

		FramebufferAttachment() = default;
		FramebufferAttachment(const FramebufferAttachmentSpecs& specs) : Specs(specs) {}
		FramebufferAttachment(const FramebufferAttachment&) = default;
	};

	

	class Framebuffer
	{
	public:
		Framebuffer(VkRenderPass renderPass, uint32_t width, uint32_t height, std::vector<FramebufferAttachmentSpecs>& specs);

		inline VkFramebuffer Handle() { return m_Handle; }
		inline VkImageView ImageView() { return m_ImageView; }

		inline std::vector<VkAttachmentDescription> Descriptions() 
		{
			std::vector<VkAttachmentDescription> ret(m_Attachments.size());
			for (uint32_t i = 0; i < ret.size(); i++)
				ret[i] = m_Attachments[i].Specs.Description;
			return ret;
		}

		inline std::vector<VkAttachmentReference> References()
		{
			std::vector<VkAttachmentReference> ret(m_Attachments.size());
			for (uint32_t i = 0; i < ret.size(); i++)
				ret[i] = m_Attachments[i].Specs.Reference;
			return ret;
		}

		inline FramebufferAttachment GetAttachment(AttachmentType type, uint32_t index)
		{
			int idx = 0;
			for (uint32_t i = 0; i < m_Attachments.size(); i++)
			{
				if (m_Attachments[i].Specs.Type == type)
				{
					if (idx == index)
						return m_Attachments[i];
					else
						idx++;
				}
			}

			FramebufferAttachment dummy;
			dummy.Specs.Type = AttachmentType::None;

			return dummy;
		}

	private:
		VkFramebuffer m_Handle;
		VkImageView m_ImageView;

		uint32_t m_Width;
		uint32_t m_Height;

		std::vector<FramebufferAttachment> m_Attachments;
	};
}