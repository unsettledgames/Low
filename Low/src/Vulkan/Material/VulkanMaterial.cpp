#include <Vulkan/Material/VulkanMaterial.h>
#include <Vulkan/Descriptor/DescriptorSet.h>
#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Resources/MaterialInstance.h>

namespace Low
{
	VulkanMaterial::VulkanMaterial(Ref<Low::Material> mat)
	{
		auto data = mat->UniformsDescriptions();
		
		// Create a descriptor set and a buffer for each material property
		for (auto& prop : data)
		{
			//DescriptorSetLayout layout()
		}
	}
}