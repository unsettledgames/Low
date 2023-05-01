#pragma once

namespace Low
{
	class Swapchain
	{
	public:
		Swapchain() = default;
		~Swapchain();
		Swapchain(uint32_t width, uint32_t height);

		void Invalidate(uint32_t width, uint32_t height);

		inline std::vector<VkImage> Images() { return m_Images; }
		inline std::vector<VkImageView> ImageViews() { return m_ImageViews; }
		inline VkFormat Format() { return m_ImageFormat; }

		inline glm::vec2 Extent() { return m_Extent; }

		inline operator VkSwapchainKHR() { return m_Handle; }

	private:
		void Cleanup();
		void Init(uint32_t width, uint32_t height);

	private:
		VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
		VkFormat m_ImageFormat;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		glm::vec2 m_Extent;
	};
}