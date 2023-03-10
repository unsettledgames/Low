#include <iostream>
#include <vector>

#include <Core/Debug.h>
#include <Renderer.h>

#include <vulkan/vulkan.h>

namespace Low
{

	struct RendererData
	{
		VkInstance Instance;
	} s_Data;

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
		appInfo.apiVersion = VK_API_VERSION_1_0;

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

	void Renderer::Init(const char** extensions, uint32_t nExtensions)
	{
		Debug::InitValidationLayers();
		CreateVkInstance(extensions, nExtensions);
		Debug::InitMessengers(s_Data.Instance);
	}

	void Renderer::Destroy()
	{
		Debug::Shutdown();
		vkDestroyInstance(s_Data.Instance, nullptr);
	}
}