#include <Vulkan/Material/VulkanMaterialInstance.h>
#include <Vulkan/Material/VulkanMaterial.h>
#include <Vulkan/VulkanCore.h>
#include <Hardware/Support.h>
#include <Resources/MaterialInstance.h>
#include <Vulkan/Command/CommandBuffer.h>

namespace Low
{
	VulkanMaterialInstance::VulkanMaterialInstance(Ref<MaterialInstance> matInstance, Ref<VulkanMaterial> mat) : m_Material(mat)
	{
		// Add values for the instance, save the indices
		for (auto& data : matInstance->UniformsData())
			m_AccessIndices.push_back(mat->PushValue(data.first, data.second));

		// Build push constants containing the indces
		m_PushConstantsBlock = new uint8_t[Support::GetPushConstantBlockSize(VulkanCore::PhysicalDevice())];

		// Put the vertex constants first. Then copy the fragment constants
	}
}