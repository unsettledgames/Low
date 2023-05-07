#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/VulkanCore.h>

namespace Low
{
	DescriptorSetLayout::DescriptorSetLayout(const std::initializer_list<DescriptorSetBinding>& bindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkBindings(bindings.size());
		uint32_t i = 0;
		for (auto& b : bindings)
		{
			vkBindings[i] = b.Handle;
			i++;
		}
		Init(vkBindings);
	}

	DescriptorSetLayout::DescriptorSetLayout(const std::vector<DescriptorSetBinding>& bindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkBindings(bindings.size());
		for (uint32_t i = 0; i < bindings.size(); i++)
			vkBindings[i] = bindings[i].Handle;
		Init(vkBindings);
	}

	void DescriptorSetLayout::Init(const std::vector<VkDescriptorSetLayoutBinding> bindings)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		VkPipelineLayout pipelineLayout = {};

		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(VulkanCore::Device(), &layoutInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create descriptor set layout");
	}

}