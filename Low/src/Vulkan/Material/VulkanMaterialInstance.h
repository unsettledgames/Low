#pragma once

/*
	- User starts creating a material.

		Material m;
		m.PushDesc(attrib);
		m.PushDesc(attrib1);
		m.PushDesc(attrib2);

		The order matters, because it's the order of the indices that will be passed to the push constant block. The stage at which each
		property is needed must be specified by the user.

	- The user can specify values for that material
		MaterialInstance instance(material);
		instance.SetValue(attribName, value);
		instance.SetValue(attribName, value);

	- When a value is specified, the corresponding VulkanMaterial updates

*/

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

		void* m_PushConstantsBlock;
	};
}