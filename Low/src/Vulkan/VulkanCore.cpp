#include <Vulkan/VulkanCore.h>
#include <Core/Debug.h>
#include <Vulkan/Queue.h>
#include <Hardware/Support.h>

#include <GLFW/glfw3.h>

namespace Low
{
	VkInstance			VulkanCore::s_Instance = VK_NULL_HANDLE;
	VkDevice			VulkanCore::s_Device = VK_NULL_HANDLE;
	VkPhysicalDevice	VulkanCore::s_PhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR		VulkanCore::s_WindowSurface = VK_NULL_HANDLE;

	Ref<Queue>			VulkanCore::s_GraphicsQueue = VK_NULL_HANDLE;
	Ref<Queue>			VulkanCore::s_PresentQueue = VK_NULL_HANDLE;

	VulkanCoreConfig	VulkanCore::s_Config = {};

	void VulkanCore::Init(const VulkanCoreConfig& config)
	{
		s_Config = config;

		Debug::InitValidationLayers();
		CreateInstance();
		Debug::InitMessengers(s_Instance);

		if (glfwCreateWindowSurface(s_Instance, s_Config.WindowHandle, nullptr, 
			&s_WindowSurface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface!");

		PickPhysicalDevice();
		CreateLogicalDevice();
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

		createInfo.enabledExtensionCount = s_Config.UserExtensions.size();
		createInfo.ppEnabledExtensionNames = s_Config.UserExtensions.data();

#ifndef LOW_VALIDATION_LAYERS
		createInfo.enabledLayerCount = 0;
#else
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();

#endif

		VkResult ret = vkCreateInstance(&createInfo, nullptr, &s_Instance);
		if (ret != VK_SUCCESS)
			std::cout << "Instance creation failure: " << ret << std::endl;
	}

	void VulkanCore::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = Support::GetQueueFamilyIndices(PhysicalDevice(), Surface());
		float priority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfo;
		std::set<uint32_t> queueFamilies = { indices.Graphics.value(), indices.Presentation.value() };

		VkDeviceCreateInfo createInfo = {};

		for (uint32_t family : queueFamilies)
		{
			VkDeviceQueueCreateInfo queueInfo = {};

			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = family;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &priority;

			queueCreateInfo.push_back(queueInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfo.data();
		createInfo.queueCreateInfoCount = queueCreateInfo.size();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = s_Config.LowExtensions.size();
		createInfo.ppEnabledExtensionNames = s_Config.LowExtensions.data();

#ifdef LOW_VALIDATION_LAYERS
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif

		auto err = vkCreateDevice(PhysicalDevice(), &createInfo, nullptr, &s_Device);
		if (err != VK_SUCCESS)
			std::cerr << "Failed creating logical device" << std::endl;

		VkQueue graphics, present;

		vkGetDeviceQueue(Device(), indices.Graphics.value(), 0, &graphics);
		vkGetDeviceQueue(Device(), indices.Presentation.value(), 0, &present);

		s_GraphicsQueue = CreateRef<Queue>(graphics, Queue::QueueType::Graphics);
		s_PresentQueue = CreateRef<Queue>(graphics, Queue::QueueType::Present);
	}

	void VulkanCore::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(Instance(), &deviceCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(Instance(), &deviceCount, physicalDevices.data());

		VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
		float currScore = -1;

		for (auto device : physicalDevices)
		{
			float score = Support::GetPhysicalDeviceScore(device, Surface(), s_Config.UserExtensions);
			if (score > currScore)
			{
				currScore = score;
				bestDevice = device;
			}
		}

		if (bestDevice == VK_NULL_HANDLE)
			std::cerr << "Couldn't find a suitable GPU" << std::endl;
		s_PhysicalDevice = bestDevice;
	}

}