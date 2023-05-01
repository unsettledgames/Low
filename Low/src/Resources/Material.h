#pragma once

namespace Low
{
	class Shader;

	enum class UniformDataType {Invalid = 0, Bool, Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Mat3, Mat4, Texture};

	struct UniformDescription
	{
		UniformDataType Type;
		bool IsPushConstant;
		std::string Name;

		UniformDescription() {}
		UniformDescription(const std::string& name, UniformDataType type, bool isPushConstant)
		{
			Type = type;
			IsPushConstant = isPushConstant;
			Name = name;
		}
	};

	class Material
	{
	public:
		Material() = default;
		Material(Ref<Shader> shader);
	
		inline void AddUniform(const UniformDescription& uniform) { m_UniformDescriptions[uniform.Name] = uniform; }
		inline void RemoveUniform(const std::string& name) { m_UniformDescriptions.erase(name); }

	private:
		Ref<Shader> m_Shader;
		std::unordered_map<std::string, UniformDescription> m_UniformDescriptions;
	};
}