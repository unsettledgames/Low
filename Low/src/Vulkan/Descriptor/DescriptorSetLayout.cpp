#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/VulkanCore.h>

namespace Low
{
	DescriptorSetLayout::DescriptorSetLayout()
	{
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

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		VkPipelineLayout pipelineLayout = {};

		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(VulkanCore::Device(), &layoutInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create descriptor set layout");
	}
}