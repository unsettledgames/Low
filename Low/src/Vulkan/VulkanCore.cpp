#include <Vulkan/VulkanCore.h>
#include <Core/Debug.h>

#include <GLFW/glfw3.h>

namespace Low
{
	struct VulkanCoreData
	{
		VkInstance Instance = VK_NULL_HANDLE;
		VkDevice Device = VK_NULL_HANDLE;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR WindowSurface = VK_NULL_HANDLE;

		VulkanCoreConfig Config = {};
	} s_VulkanCoreData;

	void VulkanCore::Init(const VulkanCoreConfig& config)
	{
		s_VulkanCoreData.Config = config;

		Debug::InitValidationLayers();
		CreateInstance();
		Debug::InitMessengers(s_VulkanCoreData.Instance);

		if (glfwCreateWindowSurface(s_VulkanCoreData.Instance, s_VulkanCoreData.Config.WindowHandle, nullptr, 
			&s_VulkanCoreData.WindowSurface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface!");

		CreateLogicalDevice();
		CreatePhysicalDevice();
	}

	void VulkanCore::CreateInstance()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> vkextensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkextensions.data());

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "LowRenderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo = {};
		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
		Debug::GetCreateInfo(debugInfo);

		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.pNext = &debugInfo;

		createInfo.enabledExtensionCount = s_VulkanCoreData.Config.Extensions.size();
		createInfo.ppEnabledExtensionNames = s_VulkanCoreData.Config.Extensions.data();

#ifndef LOW_VALIDATION_LAYERS
		createInfo.enabledLayerCount = 0;
#else
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();

#endif

		VkResult ret = vkCreateInstance(&createInfo, nullptr, &s_VulkanCoreData.Instance);
		if (ret != VK_SUCCESS)
			std::cout << "Instance creation failure: " << ret << std::endl;
	}

	VkDevice VulkanCore::Device()
	{
		return s_VulkanCoreData.Device;
	}

	VkPhysicalDevice VulkanCore::PhysicalDevice()
	{
		return s_VulkanCoreData.PhysicalDevice;
	}

	VkInstance VulkanCore::Instance()
	{
		return s_VulkanCoreData.Instance;
	}

	VkSurfaceKHR VulkanCore::Surface()
	{
		return s_VulkanCoreData.WindowSurface;
	}

}