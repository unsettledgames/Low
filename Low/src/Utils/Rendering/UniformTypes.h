#pragma once

#include <Resources/Texture.h>

namespace Low
{
	enum class DescriptorSetType { None = 0, Buffer, Sampler };
	enum class ShaderStage : uint32_t { None = 0, Vertex, Fragment, VertexFragment, Compute };

	enum class UniformDataType { Invalid = 0, Bool, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Mat3, Mat4, Texture };
	
	typedef std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4, Ref<Texture>> UniformData;

	static inline uint32_t GetShaderUniformSize(UniformDataType type)
	{
		switch (type)
		{
		case UniformDataType::Bool:		return sizeof(bool); break;
		case UniformDataType::Float:	return sizeof(float); break;
		case UniformDataType::Float2:	return sizeof(float) * 2; break;
		case UniformDataType::Float3:	return sizeof(float) * 3; break;
		case UniformDataType::Float4:	return sizeof(float) * 4; break;

		case UniformDataType::Int:		return sizeof(float); break;
		case UniformDataType::Int2:		return sizeof(float) * 2; break;
		case UniformDataType::Int3:		return sizeof(float) * 3; break;
		case UniformDataType::Int4:		return sizeof(float) * 4; break;

		case UniformDataType::Mat3:		return sizeof(float) * 3; break;
		case UniformDataType::Mat4:		return sizeof(float) * 16; break;
		default:return 0;
		}
	}

	struct DescriptorSetBinding
	{
		DescriptorSetType Type;
		ShaderStage Stage;

		uint32_t Binding;
		uint32_t Amount;

		VkDescriptorSetLayoutBinding Handle;

		DescriptorSetBinding(DescriptorSetType type, ShaderStage stage, uint32_t binding, uint32_t amount) :
			Type(type), Stage(stage), Binding(binding), Amount(amount)
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
			if (Stage == ShaderStage::Vertex || Stage == ShaderStage::VertexFragment) stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (Stage == ShaderStage::Fragment || Stage == ShaderStage::VertexFragment) stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (Stage == ShaderStage::Compute) stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

			Handle.stageFlags = stageFlags;
			Handle.pImmutableSamplers = nullptr;
		}
	};

	struct UniformDescription
	{
		UniformDataType Type;
		ShaderStage Stage;
		std::string Name;

		UniformDescription() {}
		UniformDescription(const std::string& name, UniformDataType type, ShaderStage stage) :
			Type(type), Stage(stage), Name(name) {}
		inline uint32_t Size() { return GetShaderUniformSize(Type); }
	};

	struct PushConstantDescription
	{
		std::vector<UniformDataType> Elements;
		PushConstantDescription() {}
		PushConstantDescription(const std::vector<UniformDataType>& elements) : Elements(elements) {}

		inline uint32_t Size()
		{
			uint32_t ret = 0;
			for (auto& el : Elements)
				ret += GetShaderUniformSize(el);

			return ret;
		}
	};
}