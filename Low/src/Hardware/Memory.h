#pragma once

namespace Low
{
	class Memory
	{
	public:
		static uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter, VkMemoryPropertyFlags props);
	};
}