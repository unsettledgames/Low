#pragma once

namespace Low
{
	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout();

		inline operator VkDescriptorSetLayout() { return m_Handle; }

	private:
		VkDescriptorSetLayout m_Handle;
	};
}