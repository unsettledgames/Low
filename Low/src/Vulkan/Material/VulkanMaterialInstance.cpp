#include <Vulkan/Material/VulkanMaterialInstance.h>
#include <Vulkan/Material/VulkanMaterial.h>
#include <Resources/MaterialInstance.h>

namespace Low
{
	VulkanMaterialInstance::VulkanMaterialInstance(Ref<MaterialInstance> matInstance, Ref<VulkanMaterial> mat) : m_Material(mat)
	{
		for (auto& data : matInstance->UniformsData())
		{
			/*uint32_t index = */mat->PushValue(data.first, data.second);
		}
	}
}