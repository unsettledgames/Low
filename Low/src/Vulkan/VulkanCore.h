#pragma once

struct GLFWwindow;

namespace Low
{
	struct VulkanCoreConfig
	{
		std::vector<const char*> Extensions;
		GLFWwindow* WindowHandle;
	};

	class VulkanCore
	{
	public:
		static VkInstance Instance();
		static VkDevice Device();
		static VkPhysicalDevice PhysicalDevice();
		static VkSurfaceKHR Surface();

		static void Init(const VulkanCoreConfig& config);

	private:
		static void CreateInstance();
		static void PickPhysicalDevice();
		static void CreateLogicalDevice();
	};
}