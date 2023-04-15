#include <Core/Debug.h>
#include <iostream>

namespace Low
{
	std::vector<const char*> Debug::s_ValidationLayers;

	struct DebugData
	{
		VkDebugUtilsMessengerEXT DebugMessenger;
		std::vector<const char*> ValidationLayers;

		VkInstance ReferenceInstance;
	} s_DebugData;

	static bool ValidationLayersSupported()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (auto layer : s_DebugData.ValidationLayers)
		{
			bool found = false;
			for (auto& props : availableLayers)
			{
				if (std::string(props.layerName) == std::string(layer))
				{
					found = true;
					break;
				}
			}

			if (!found)
				return false;
		}

		return true;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	static void SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		Debug::GetCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(s_DebugData.ReferenceInstance, &createInfo, nullptr, &s_DebugData.DebugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void Debug::InitValidationLayers()
	{
#ifdef LOW_VALIDATION_LAYERS
		s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		s_DebugData.ValidationLayers = s_ValidationLayers;

		if (!ValidationLayersSupported())
			std::cout << "Unsupported validation layers" << std::endl << std::endl;
#endif
	}

	void Debug::InitMessengers(VkInstance instance)
	{
#ifdef LOW_VALIDATION_LAYERS
		s_DebugData.ReferenceInstance = instance;
		SetupDebugMessenger();
#endif
	}

	void Debug::Shutdown()
	{
#ifdef LOW_VALIDATION_LAYERS
		DestroyDebugUtilsMessengerEXT(s_DebugData.ReferenceInstance, s_DebugData.DebugMessenger, nullptr);
#endif
	}
}