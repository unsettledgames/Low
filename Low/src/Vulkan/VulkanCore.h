#pragma once

struct GLFWwindow;

namespace Low
{
	struct VulkanCoreConfig
	{
		std::vector<const char*> UserExtensions;
		std::vector<const char*> LowExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		GLFWwindow* WindowHandle;
	};

	class Queue;

	class VulkanCore
	{
	public:
		static VkInstance Instance();
		static VkDevice Device();
		static VkPhysicalDevice PhysicalDevice();
		static VkSurfaceKHR Surface();

		static Ref<Queue> GraphicsQueue();
		static Ref<Queue> PresentQueue();

		static void Init(const VulkanCoreConfig& config);

	private:
		static void CreateInstance();
		static void PickPhysicalDevice();
		static void CreateLogicalDevice();
	};
}