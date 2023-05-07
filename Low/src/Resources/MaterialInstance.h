#pragma once

#include <Resources/Material.h>
#include <Utils/Rendering/UniformTypes.h>

namespace Low
{
	class MaterialInstance
	{
	public:
		MaterialInstance() = default;
		~MaterialInstance() = default;

		MaterialInstance(const MaterialInstance&) = default;
		MaterialInstance(const Material& material) : m_Material(material) {}
		
		inline void SetUniform(const std::string& name, const UniformData& uniform) { m_UniformData[name] = uniform; }
		inline void SetPushConstantBlock(void* data) { m_PushConstantData = data; }

		inline Material GetMaterial() { return m_Material; }
		inline std::unordered_map<std::string, UniformData> UniformsData() { return m_UniformData; }

		inline UUID ID() { return m_ID; }

	private:
		Low::Material m_Material;
		UUID m_ID;
		std::unordered_map<std::string, UniformData> m_UniformData;
		void* m_PushConstantData;
	};
}