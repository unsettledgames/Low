#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/VulkanCore.h>

namespace Low
{
	/*
		VkDescriptorSetLayoutBinding uboBinding = {};
		uboBinding.binding = 0;
		uboBinding.descriptorCount = 1;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerBinding = {};
		samplerBinding.binding = 1;
		samplerBinding.descriptorCount = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding sampler2 = {};
		sampler2.binding = 2;
		sampler2.descriptorCount = 1;
		sampler2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler2.pImmutableSamplers = nullptr;
		sampler2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboBinding, samplerBinding, sampler2};
	*/

	DescriptorSetLayout::DescriptorSetLayout(const std::initializer_list<DescriptorSetBinding>& bindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkBindings(bindings.size());
		uint32_t i = 0;
		for (auto& b : bindings)
		{
			vkBindings[i] = b.Handle;
			i++;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		VkPipelineLayout pipelineLayout = {};

		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = vkBindings.size();
		layoutInfo.pBindings = vkBindings.data();

		if (vkCreateDescriptorSetLayout(VulkanCore::Device(), &layoutInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create descriptor set layout");
	}
}