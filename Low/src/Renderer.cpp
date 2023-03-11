#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>

#include <Core/Debug.h>
#include <Renderer.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


// This stuff should probably go in a VulkanCore class or something. Then expose globals init and shutdown that initialize and shutdown
// everything. Instances, devices and stuff like that don't have much to do with the actual renderer

namespace Low
{

	struct RendererData
	{
		VkInstance Instance = VK_NULL_HANDLE;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkDevice LogicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR WindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR SwapChain = VK_NULL_HANDLE;

		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		VkQueue PresentationQueue = VK_NULL_HANDLE;

		GLFWwindow* WindowHandle;
		std::vector<const char*> RequiredExtesions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	} s_Data;

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

	static SwapchainSupportDetails GetSwapchainSupportDetails(VkPhysicalDevice device)
	{
		SwapchainSupportDetails ret;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, s_Data.WindowSurface, &ret.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_Data.WindowSurface, &formatCount, nullptr);
		ret.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_Data.WindowSurface, &formatCount, ret.Formats.data());

		uint32_t presentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_Data.WindowSurface, &presentModesCount, nullptr);
		ret.PresentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_Data.WindowSurface, &presentModesCount, ret.PresentModes.data());

		return ret;
	}

	static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device)
	{
		QueueFamilyIndices ret;

		uint32_t nQueues;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueues, nullptr);
		std::vector<VkQueueFamilyProperties> queueProps(nQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueues, queueProps.data());

		uint32_t i = 0;

		for (auto family : queueProps)
		{
			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				ret.Graphics = i;

			VkBool32 present;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, s_Data.WindowSurface, &present);
			if (present)
				ret.Presentation = i;

			if (ret.Graphics.has_value() && ret.Presentation.has_value())
				break;
			i++;
		}

		return ret;
	}

	static void CreateVkInstance(const char** extensions, uint32_t nExtensions)
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> vkextensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkextensions.data());
		std::cout << "available extensions:\n";

		for (const auto& extension : vkextensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}

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

		createInfo.enabledExtensionCount = nExtensions;
		createInfo.ppEnabledExtensionNames = extensions;

#ifndef LOW_VALIDATION_LAYERS
		createInfo.enabledLayerCount = 0;
#else
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();

#endif

		VkResult ret = vkCreateInstance(&createInfo, nullptr, &s_Data.Instance);
		if (ret != VK_SUCCESS)
			std::cout << "Instance creation failure: " << ret << std::endl;
	}

	static float GetPhysicalDeviceScore(VkPhysicalDevice device)
	{
		float ret = 0;
		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memoryProps;

		vkGetPhysicalDeviceProperties(device, &props);
		vkGetPhysicalDeviceFeatures(device, &features);
		vkGetPhysicalDeviceMemoryProperties(device, &memoryProps);

		// GPU properties
		if (props.deviceType = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			ret += 1000.0f;
		for (uint32_t i = 0; i < memoryProps.memoryHeapCount; i++)
			ret += memoryProps.memoryHeaps[i].size / 1000000;
		
		// Queues
		auto queueIndices = GetQueueFamilyIndices(device);
		if (!queueIndices.Complete())
			return 0;
		if (queueIndices.Graphics.value() != queueIndices.Presentation.value())
			ret -= 100.0f;

		// Extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<const char*> requiredExtensions(s_Data.RequiredExtesions.begin(), s_Data.RequiredExtesions.end());

		for (auto ext : availableExtensions)
			requiredExtensions.erase(ext.extensionName);
		if (!requiredExtensions.empty())
			return 0;

		return ret;
	}

	static void PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(s_Data.Instance, &deviceCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(s_Data.Instance, &deviceCount, physicalDevices.data());

		VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
		float currScore = -1;

		for (auto device : physicalDevices)
		{
			float score = GetPhysicalDeviceScore(device);
			if (score > currScore)
			{
				currScore = score;
				bestDevice = device;
			}
		}

		if (bestDevice == VK_NULL_HANDLE)
			std::cerr << "Couldn't find a suitable GPU" << std::endl;
		s_Data.PhysicalDevice = bestDevice;
	}

	static void CreateLogicalDevice()
	{
		QueueFamilyIndices indices = GetQueueFamilyIndices(s_Data.PhysicalDevice);
		float priority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfo;
		std::set<uint32_t> queueFamilies = { indices.Graphics.value(), indices.Presentation.value()};

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

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfo.data();
		createInfo.queueCreateInfoCount = queueCreateInfo.size();
		createInfo.pEnabledFeatures = &deviceFeatures;
		
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = s_Data.RequiredExtesions.data();

#ifdef LOW_VALIDATION_LAYERS
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif

		auto err = vkCreateDevice(s_Data.PhysicalDevice, &createInfo, nullptr, &s_Data.LogicalDevice);
		if (err != VK_SUCCESS)
			std::cerr << "Failed creating logical device" << std::endl;

		vkGetDeviceQueue(s_Data.LogicalDevice, indices.Graphics.value(), 0, &s_Data.GraphicsQueue);
		vkGetDeviceQueue(s_Data.LogicalDevice, indices.Presentation.value(), 0, &s_Data.PresentationQueue);
	}

	static void CreateWindowSurface()
	{
		if (glfwCreateWindowSurface(s_Data.Instance, s_Data.WindowHandle, nullptr, &s_Data.WindowSurface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface!");
	}

	static void CreateSwapChain()
	{
		// Swapchain properties
		SwapchainSupportDetails swapchainProps = GetSwapchainSupportDetails(s_Data.PhysicalDevice);
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
			int width, height;
			glfwGetFramebufferSize(s_Data.WindowHandle, &width, &height);
			extent = { (uint32_t)width, (uint32_t)height };
			extent.width = std::clamp(extent.width, swapchainProps.Capabilities.minImageExtent.width, swapchainProps.Capabilities.maxImageExtent.width);
			extent.height = std::clamp(extent.height, swapchainProps.Capabilities.minImageExtent.height, swapchainProps.Capabilities.maxImageExtent.height);
		}

		uint32_t imageCount = swapchainProps.Capabilities.minImageCount;
		if (imageCount != swapchainProps.Capabilities.maxImageCount)
			imageCount++;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = s_Data.WindowSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = swapchainFormat.format;
		createInfo.imageColorSpace = swapchainFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto indice = GetQueueFamilyIndices(s_Data.PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indice.Graphics.value(), indice.Presentation.value() };

		if (indice.Graphics != indice.Presentation) 
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

		VkResult res = vkCreateSwapchainKHR(s_Data.LogicalDevice, &createInfo, nullptr, &s_Data.SwapChain);
		if (res != VK_SUCCESS)
			std::cerr << "Couldn't create swapchain" << std::endl;
	}

	void Renderer::Init(const char** extensions, uint32_t nExtensions, GLFWwindow* handle)
	{
		s_Data.WindowHandle = handle;

		Debug::InitValidationLayers();

		CreateVkInstance(extensions, nExtensions);
		Debug::InitMessengers(s_Data.Instance);

		CreateWindowSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();

		CreateSwapChain();
	}

	void Renderer::Destroy()
	{
		Debug::Shutdown();

		vkDestroySwapchainKHR(s_Data.LogicalDevice, s_Data.SwapChain, nullptr);

		vkDestroyDevice(s_Data.LogicalDevice, nullptr);

		vkDestroySurfaceKHR(s_Data.Instance, s_Data.WindowSurface, nullptr);

		vkDestroyInstance(s_Data.Instance, nullptr);

	}
}