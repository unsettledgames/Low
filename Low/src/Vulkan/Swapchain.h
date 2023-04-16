#pragma once

namespace Low
{
	class Swapchain
	{
	public:
		Swapchain() = default;
		Swapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height);

		inline VkSwapchainKHR Handle() { return m_Handle; }
		inline std::vector<VkImageView> ImageViews() { return m_ImageViews; }
		inline VkFormat Format() { return m_ImageFormat; }

	private:
		void CreateImages();
		void CreateImageViews();

	private:
		VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
		VkFormat m_ImageFormat;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		glm::vec2 m_Extent;
	};
}