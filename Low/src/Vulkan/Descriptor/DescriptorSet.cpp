#include <Vulkan/Descriptor/DescriptorSet.h>
#include <Vulkan/Descriptor/DescriptorPool.h>
#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/VulkanCore.h>
#include <Resources/Material.h>
#include <Resources/MaterialInstance.h>

#include <Core/State.h>

namespace Low
{
	DescriptorSet::DescriptorSet(const DescriptorSetLayout& layout, uint32_t amount)
	{
		// [TODO]: defer descriptor set allocation
		std::vector<VkDescriptorSetLayout> layouts = { layout };

		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = *VulkanCore::DescriptorPool();
		allocateInfo.descriptorSetCount = amount;
		allocateInfo.pSetLayouts = layouts.data();

		vkAllocateDescriptorSets(VulkanCore::Device(), &allocateInfo, &m_Handle);
	}
}