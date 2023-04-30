#pragma once

namespace Low
{
	class DescriptorPool
	{
	public:
		DescriptorPool(uint32_t count);

		inline operator VkDescriptorPool() { return m_Handle; }
	private:
		VkDescriptorPool m_Handle;
	};
}