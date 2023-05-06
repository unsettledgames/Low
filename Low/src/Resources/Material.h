#pragma once

namespace Low
{
	class Shader;

	enum class UniformDataType {Invalid = 0, Bool, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Mat3, Mat4, Texture};

	struct UniformDescription
	{
		UniformDataType Type;
		std::string Name;

		bool MemoryMapped;

		UniformDescription() {}
		UniformDescription(const std::string& name, UniformDataType type, bool memoryMapped)
		{
			Type = type;
			Name = name;
			MemoryMapped = memoryMapped;
		}
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
			{
				switch (el)
				{
				case UniformDataType::Bool:		ret += sizeof(bool); break;
				case UniformDataType::Float:	ret += sizeof(float); break;
				case UniformDataType::Float2:	ret += sizeof(float) * 2; break;
				case UniformDataType::Float3:	ret += sizeof(float) * 3; break;
				case UniformDataType::Float4:	ret += sizeof(float) * 4; break;

				case UniformDataType::Int:		ret += sizeof(float); break;
				case UniformDataType::Int2:		ret += sizeof(float) * 2; break;
				case UniformDataType::Int3:		ret += sizeof(float) * 3; break;
				case UniformDataType::Int4:		ret += sizeof(float) * 4; break;

				case UniformDataType::Mat3:		ret += sizeof(float) * 3; break;
				case UniformDataType::Mat4:		ret += sizeof(float) * 16; break;
				default:break;
				}
			}

			return ret;
		}
	};

	class Material
	{
	public:
		Material() = default;
		Material(Ref<Shader> shader);
	
		inline void AddUniform(const UniformDescription& uniform) { m_UniformDescriptions[uniform.Name] = uniform; }
		inline void SetPushConstantBlok(const PushConstantDescription& pcb) { m_PushConstantBlockDescription = pcb; }

		inline std::unordered_map<std::string, UniformDescription> UniformsDescriptions() { return m_UniformDescriptions; }

	private:
		Ref<Shader> m_Shader;
		std::unordered_map<std::string, UniformDescription> m_UniformDescriptions;
		PushConstantDescription m_PushConstantBlockDescription;
	};
}