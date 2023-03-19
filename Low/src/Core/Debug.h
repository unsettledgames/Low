#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

#define LOW_VALIDATION_LAYERS

namespace Low
{
	class Debug
	{
	public:
		static void InitValidationLayers();
		static void InitMessengers(VkInstance instance);
		static void Shutdown();

		static inline std::vector<const char*> GetValidationLayers() 
		{ 
			return s_ValidationLayers; 
		}

		static inline void GetCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
		{
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = DebugCallback;
			createInfo.pUserData = nullptr;
		}

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{

			std::cerr << pCallbackData->pMessage << std::endl << std::endl;
			return VK_FALSE;
		}

	private:
		static std::vector<const char*> s_ValidationLayers;
	};
}