#pragma once

namespace Low
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> Graphics;
		std::optional<uint32_t> Presentation;

		bool Complete() { return Graphics.has_value() && Presentation.has_value(); }
	};

	class Support
	{
	public:
		static float GetPhysicalDeviceScore(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& requiredExtensions);

		static QueueFamilyIndices Support::GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);
	};
}