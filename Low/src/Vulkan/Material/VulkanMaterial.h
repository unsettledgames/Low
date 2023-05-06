#pragma once

#include <Vulkan/Descriptor/DescriptorSet.h>
#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Resources/MaterialInstance.h>

namespace Low
{
	class Material;
	class DescriptorSet;
	class DescriptorSetLayout;
	class Buffer;

	class VulkanMaterial
	{
	public:
		VulkanMaterial(Ref<Material> material);

		void PushValue(const std::string& name, const UniformData& value) {}

		std::vector<DescriptorSet> DescriptorSets() { return m_DescriptorSets; }
		std::vector<Ref<Buffer>> Buffers() { return m_Buffers; }
		std::vector<DescriptorSetLayout> Layouts() { return m_Layouts; }

		Ref<Material> GetMaterial() { return m_Material; }

	private:
		std::vector<VkWriteDescriptorSet> m_DescriptorWrites;
		std::vector<DescriptorSet> m_DescriptorSets;
		std::vector<Ref<Buffer>> m_Buffers;
		std::vector<DescriptorSetLayout> m_Layouts;

		Ref<Material> m_Material;
	};
}