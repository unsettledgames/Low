#pragma once
#include <variant>
#include <Resources/Material.h>

namespace Low
{
	typedef std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4> UniformData;

	class MaterialInstance
	{
	public:
		MaterialInstance() = default;
		~MaterialInstance() = default;

		MaterialInstance(const MaterialInstance&) = default;
		MaterialInstance(const Material& material) : m_Material(material) {}
		
		inline void SetUniform(const std::string& name, const UniformData& uniform) { m_UniformData[name] = uniform; }

		inline UUID ID() { return m_ID; }

	private:
		Material m_Material;
		UUID m_ID;
		std::unordered_map<std::string, UniformData> m_UniformData;
	};
}