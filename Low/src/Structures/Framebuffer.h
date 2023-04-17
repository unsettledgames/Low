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
		uint32_t SampleCount = 0;

		FramebufferAttachmentConfig() = default;
		// Swapchain attachment
		FramebufferAttachmentConfig(AttachmentType type, VkImage image, uint32_t width, uint32_t height, VkFormat format) :
			Type(type), Image(image), Width(width), Height(height), Format(format) {}
		// User defined attachment
		FramebufferAttachmentConfig(AttachmentType type, uint32_t width, uint32_t height, VkFormat format, uint32_t nSamples = 1) :
			Type(type), Width(width), Height(height), SampleCount(nSamples) {}
	};

	struct FramebufferAttachment
	{
		VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		
		VkAttachmentDescription Description;
		VkAttachmentReference Reference;

		FramebufferAttachment() = default;
		FramebufferAttachment(const FramebufferAttachment&) = default;
	};

	struct FramebufferConfig
	{
		std::vector<FramebufferAttachmentConfig> Attachments;

		FramebufferConfig(const std::vector<FramebufferAttachmentConfig> attachments) :
			Attachments(attachments) {}
	};

	class Framebuffer
	{
	public:
		Framebuffer(FramebufferConfig& config);

		inline VkFramebuffer Handle() { return m_Handle; }
		inline VkImageView ImageView() { return m_ImageView; }

	private:
		VkFramebuffer m_Handle;
		VkImageView m_ImageView;

		std::vector<FramebufferAttachment> m_Attachments;
	};
}