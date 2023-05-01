#include <Hardware/Support.h>
#include <Vulkan/VulkanCore.h>
#include <Vulkan/Swapchain.h>

namespace Low
{
	Swapchain::Swapchain(uint32_t width, uint32_t height)
	{
		Init(width, height);
	}

	Swapchain::~Swapchain()
	{
		Cleanup();
	}

	void Swapchain::Invalidate(uint32_t width, uint32_t height)
	{
		Cleanup();
		Init(width, height);
	}

	void Swapchain::Init(uint32_t width, uint32_t height)
	{
		m_Extent = { width, height };
		// Swapchain properties
		SwapchainSupportDetails swapchainProps = Support::GetSwapchainSupportDetails(VulkanCore::PhysicalDevice(), VulkanCore::Surface());
		if (swapchainProps.Formats.empty() || swapchainProps.PresentModes.empty())
			throw std::runtime_error("Swapchain has no formats or present modes");

		// Color format
		VkSurfaceFormatKHR swapchainFormat;
		bool foundFormat = false;

		for (auto format : swapchainProps.Formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				swapchainFormat = format;
				foundFormat = true;
				break;
			}
		}
		// For lack of better formats, use the first one
		if (!foundFormat)
			swapchainFormat = swapchainProps.Formats[0];
		m_ImageFormat = swapchainFormat.format;

		// Presentation mode
		VkPresentModeKHR presentMode;
		bool foundPresentation = false;

		for (auto presentation : swapchainProps.PresentModes)
		{
			if (presentation == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = presentation;
				foundPresentation = true;
				break;
			}
		}
		// Same concept as the format
		if (!foundPresentation)
			presentMode = VK_PRESENT_MODE_FIFO_KHR;

		VkExtent2D extent;

		// Swap extent
		if (swapchainProps.Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			extent = swapchainProps.Capabilities.currentExtent;
		else
		{
			extent = { (uint32_t)width, (uint32_t)height };
			extent.width = std::clamp(extent.width, swapchainProps.Capabilities.minImageExtent.width, swapchainProps.Capabilities.maxImageExtent.width);
			extent.height = std::clamp(extent.height, swapchainProps.Capabilities.minImageExtent.height, swapchainProps.Capabilities.maxImageExtent.height);
		}

		uint32_t imageCount = swapchainProps.Capabilities.minImageCount;
		if (imageCount != swapchainProps.Capabilities.maxImageCount)
			imageCount++;
		m_Images.resize(imageCount);

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = VulkanCore::Surface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = swapchainFormat.format;
		createInfo.imageColorSpace = swapchainFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto indices = Support::GetQueueFamilyIndices(VulkanCore::PhysicalDevice(), createInfo.surface);
		uint32_t queueFamilyIndices[] = { indices.Graphics.value(), indices.Presentation.value() };

		if (indices.Graphics != indices.Presentation)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapchainProps.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult res = vkCreateSwapchainKHR(VulkanCore::Device(), &createInfo, nullptr, &m_Handle);
		if (res != VK_SUCCESS)
			std::cerr << "Couldn't create swapchain" << std::endl;

		uint32_t size = m_Images.size();
		vkGetSwapchainImagesKHR(VulkanCore::Device(), m_Handle, &size, m_Images.data());
	}

	void Swapchain::Cleanup()
	{
		for (auto& image : m_ImageViews)
			vkDestroyImageView(VulkanCore::Device(), image, nullptr);
		vkDestroySwapchainKHR(VulkanCore::Device(), m_Handle, nullptr);
	}
}