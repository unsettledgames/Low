#pragma once

#include <Lowpch.h>

struct GLFWwindow;

namespace Low
{
	struct VulkanCoreConfig
	{
		std::vector<char*> Extensions;
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
		static void CreatePhysicalDevice();
		static void CreateLogicalDevice();
	};
}