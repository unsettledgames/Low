#pragma once

namespace Low
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> Graphics;
		std::optional<uint32_t> Presentation;

		bool Complete() { return Graphics.has_value() && Presentation.has_value(); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	class Support
	{
	public:
		static float GetPhysicalDeviceScore(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& requiredExtensions);

		static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

		static SwapchainSupportDetails GetSwapchainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
	};
}