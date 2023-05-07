#pragma once

#include <Utils/Rendering/UniformTypes.h>

namespace Low
{
	class Shader;	

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