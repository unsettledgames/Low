#include <Vulkan/VulkanCore.h>
#include <Core/Debug.h>
#include <Vulkan/Queue.h>
#include <Hardware/Support.h>

#include <GLFW/glfw3.h>

namespace Low
{
	struct VulkanCoreData
	{
		VkInstance Instance = VK_NULL_HANDLE;
		VkDevice Device = VK_NULL_HANDLE;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR WindowSurface = VK_NULL_HANDLE;
		
		Ref<Queue> GraphicsQueue = VK_NULL_HANDLE;
		Ref<Queue> PresentQueue = VK_NULL_HANDLE;

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

		createInfo.enabledExtensionCount = s_VulkanCoreData.Config.UserExtensions.size();
		createInfo.ppEnabledExtensionNames = s_VulkanCoreData.Config.UserExtensions.data();

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

		createInfo.enabledExtensionCount = s_VulkanCoreData.Config.LowExtensions.size();
		createInfo.ppEnabledExtensionNames = s_VulkanCoreData.Config.LowExtensions.data();

#ifdef LOW_VALIDATION_LAYERS
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif

		auto err = vkCreateDevice(PhysicalDevice(), &createInfo, nullptr, &s_VulkanCoreData.Device);
		if (err != VK_SUCCESS)
			std::cerr << "Failed creating logical device" << std::endl;

		VkQueue graphics, present;

		vkGetDeviceQueue(Device(), indices.Graphics.value(), 0, &graphics);
		vkGetDeviceQueue(Device(), indices.Presentation.value(), 0, &present);

		s_VulkanCoreData.GraphicsQueue = CreateRef<Queue>(graphics, Queue::QueueType::Graphics);
		s_VulkanCoreData.PresentQueue = CreateRef<Queue>(graphics, Queue::QueueType::Present);
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
			float score = Support::GetPhysicalDeviceScore(device, Surface(), s_VulkanCoreData.Config.UserExtensions);
			if (score > currScore)
			{
				currScore = score;
				bestDevice = device;
			}
		}

		if (bestDevice == VK_NULL_HANDLE)
			std::cerr << "Couldn't find a suitable GPU" << std::endl;
		s_VulkanCoreData.PhysicalDevice = bestDevice;
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

	Ref<Queue> VulkanCore::GraphicsQueue()
	{
		return s_VulkanCoreData.GraphicsQueue;
	}

	Ref<Queue>VulkanCore::PresentQueue()
	{
		return s_VulkanCoreData.PresentQueue;
	}

}