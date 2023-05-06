#pragma once

namespace Low
{
	class VulkanMaterial;
	class MaterialInstance;

	class VulkanMaterialInstance
	{
	public:
		VulkanMaterialInstance(Ref<MaterialInstance> instance, Ref<VulkanMaterial> material);

		Ref<VulkanMaterial> VkMaterial() { return m_Material; }
	private:
		std::vector<uint32_t> m_AccessIndices;

		Ref<VulkanMaterial> m_Material;
	};
}