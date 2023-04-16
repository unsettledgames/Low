#pragma once

namespace Low
{
	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout();

		inline VkDescriptorSetLayout Handle() { return m_Handle; }

	private:
		VkDescriptorSetLayout m_Handle;
	};
}