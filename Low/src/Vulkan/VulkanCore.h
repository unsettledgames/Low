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
		static inline VkInstance Instance() { return s_Instance; }
		static inline VkDevice Device() { return s_Device; }
		static inline VkPhysicalDevice PhysicalDevice() { return s_PhysicalDevice; }
		static inline VkSurfaceKHR Surface() { return s_WindowSurface; }
		
		static inline Ref<Queue> GraphicsQueue() { return s_GraphicsQueue; }
		static inline Ref<Queue> PresentQueue() { return s_PresentQueue; }

		static void Init(const VulkanCoreConfig& config);

	private:
		static void CreateInstance();
		static void PickPhysicalDevice();
		static void CreateLogicalDevice();

	private:
		static VkInstance s_Instance;
		static VkDevice s_Device;
		static VkPhysicalDevice s_PhysicalDevice;
		static VkSurfaceKHR s_WindowSurface;

		static Ref<Queue> s_GraphicsQueue;
		static Ref<Queue> s_PresentQueue;

		static VulkanCoreConfig s_Config;
	};
}