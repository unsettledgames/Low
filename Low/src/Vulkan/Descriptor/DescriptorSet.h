#pragma once

namespace Low
{
	class DescriptorSetLayout;
	class DescriptorPool;

	class DescriptorSet
	{
	public:
		DescriptorSet() = default;
		DescriptorSet(const DescriptorSetLayout& layout, uint32_t amount);
		operator VkDescriptorSet() const { return m_Handle; }

	private:
		VkDescriptorSet m_Handle;
	};
}