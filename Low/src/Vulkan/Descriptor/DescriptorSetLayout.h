#pragma once

#include <Utils/Rendering/UniformTypes.h>

namespace Low
{
	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout(const std::vector<DescriptorSetBinding>& bindings);
		DescriptorSetLayout(const std::initializer_list<DescriptorSetBinding>& bindings);

		inline operator VkDescriptorSetLayout() const { return m_Handle; }

	private:
		void Init(const std::vector<VkDescriptorSetLayoutBinding> bindings);

	private:
		VkDescriptorSetLayout m_Handle;
	};
}