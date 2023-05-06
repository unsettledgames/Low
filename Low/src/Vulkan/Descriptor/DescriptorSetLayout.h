#pragma once

namespace Low
{
	enum class DescriptorSetType {None = 0, Buffer, Sampler};
	enum class DescriptorStage : uint32_t {None = 0, Vertex = 1, Fragment = 2, Compute = 4};

	typedef uint32_t DescriptorStageFlags;

	struct DescriptorSetBinding
	{
		DescriptorSetType Type;
		DescriptorStageFlags Stages;

		uint32_t Binding;
		uint32_t Amount;

		VkDescriptorSetLayoutBinding Handle;

		DescriptorSetBinding(DescriptorSetType type, DescriptorStageFlags stage, uint32_t binding, uint32_t amount) :
			Type(type), Stages(stage), Binding(binding), Amount(amount)
		{
			Handle.binding = Binding;
			Handle.descriptorCount = Amount;
			switch (Type)
			{
			case DescriptorSetType::Buffer: Handle.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
			case DescriptorSetType::Sampler: Handle.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
			default:break;
			}

			VkShaderStageFlags stageFlags = 0;
			if (Stages & (uint32_t)DescriptorStage::Vertex) stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (Stages & (uint32_t)DescriptorStage::Fragment) stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (Stages & (uint32_t)DescriptorStage::Compute) stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

			Handle.stageFlags = stageFlags;
			Handle.pImmutableSamplers = nullptr;
		}
	};

	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout(const std::initializer_list<DescriptorSetBinding>& bindings);

		inline operator VkDescriptorSetLayout() const { return m_Handle; }
		inline VkDescriptorSetLayout Handle() const { return m_Handle; }

	private:
		VkDescriptorSetLayout m_Handle;
	};
}