#pragma once

namespace Low
{
	class Shader
	{
	public:
		Shader(const std::string& shaderName);
		~Shader();

		inline VkShaderModule GetVertexModule() { return m_VertModule; }
		inline VkShaderModule GetFragmentModule() { return m_FragModule; }

	private:
		void Compile(const std::string& vertSrc, const std::string& fragSrc, std::vector<uint32_t>& vertBin, std::vector<uint32_t>& fragBin);
		void CreateVkShader(const std::vector<uint32_t>& vertSpirv, const std::vector<uint32_t>& fragSpirv);
	private:
		std::string m_Name;

		VkShaderModule m_VertModule;
		VkShaderModule m_FragModule;
	};
}