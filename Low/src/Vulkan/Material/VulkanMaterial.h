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

		uint32_t PushValue(const std::string& name, const UniformData& value);

		Ref<DescriptorSet> DescriptorSet() { return WrapRef<Low::DescriptorSet>(&m_DescriptorSet); }
		std::unordered_map<std::string, Ref<Buffer>> Buffers() { return m_Buffers; }
		std::vector<DescriptorSetLayout> Layouts() { return m_Layouts; }

		Ref<Material> GetMaterial() { return m_Material; }

	private:
		std::vector<VkWriteDescriptorSet> m_DescriptorWrites;
		Low::DescriptorSet m_DescriptorSet;
		std::vector<DescriptorSetLayout> m_Layouts;

		std::unordered_map<std::string, Ref<Buffer>> m_Buffers;
		// Precompute and cash hash to save time?
		std::unordered_map<std::string, void*> m_BuffersMapped;
		std::unordered_map<std::string, uint32_t> m_BuffersLastIndex;

		Ref<Material> m_Material;
	};
}