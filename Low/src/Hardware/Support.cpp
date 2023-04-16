#include <Hardware/Support.h>

namespace Low
{
	float Support::GetPhysicalDeviceScore(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& requiredExtensions)
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
		if (features.samplerAnisotropy)
			ret += 1.0f;

		for (uint32_t i = 0; i < memoryProps.memoryHeapCount; i++)
			ret += memoryProps.memoryHeaps[i].size / 1000000;

		// Queues
		auto queueIndices = GetQueueFamilyIndices(device, surface);
		if (!queueIndices.Complete())
			return 0;
		if (queueIndices.Graphics.value() != queueIndices.Presentation.value())
			ret -= 100.0f;

		// Extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<const char*> editableExt(requiredExtensions.begin(), requiredExtensions.end());

		for (auto ext : availableExtensions)
			editableExt.erase(ext.extensionName);
		if (!editableExt.empty())
			return 0;

		return ret;
	}

	QueueFamilyIndices Support::GetQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present);
			if (present)
				ret.Presentation = i;

			if (ret.Graphics.has_value() && ret.Presentation.has_value())
				break;
			i++;
		}

		return ret;
	}
}