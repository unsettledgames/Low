#include <Resources/Shader.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <shaderc/shaderc.hpp>

// TODO: store bytecode somewhere instead of reloading it from scratch every time
// https://gist.github.com/sortofsleepy/a7fcb8221f70cab37e82f3779e78aaa5

namespace Low
{
	Shader::Shader(const std::string& shaderName, VkDevice device) : m_Name(shaderName), m_Device(device)
	{
		std::string vertPath = "../../Assets/Shaders/" + shaderName + ".vert";
		std::string fragPath = "../../Assets/Shaders/" + shaderName + ".frag";

		std::ifstream vertFile(vertPath), fragFile(fragPath);
		if (!vertFile.is_open())
			throw std::runtime_error("Coudln't find vertex shader file");
		if (!fragFile.is_open())
			throw std::runtime_error("Coudln't find fragment shader file");

		if (!vertFile.good())
			std::cout << "Vert not good" << std::endl;

		// Get shader source
		std::string vertSource, fragSource;
		std::stringstream ss;
		ss << vertFile.rdbuf();
		vertSource = ss.str();

		ss.str("");
		ss << fragFile.rdbuf();
		fragSource = ss.str();

		// Compile and link
		std::vector<uint32_t> vertSpirv, fragSpirv;
		Compile(vertSource, fragSource, vertSpirv, fragSpirv);

		// Create actual vulkan shader
		CreateVkShader(vertSpirv, fragSpirv);
	}

	Shader::~Shader()
	{
		vkDestroyShaderModule(m_Device, m_FragModule, nullptr);
		vkDestroyShaderModule(m_Device, m_VertModule, nullptr);
	}

	void Shader::Compile(const std::string& vertSrc, const std::string& fragSrc, std::vector<uint32_t>& vertBin, std::vector<uint32_t>& fragBin)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options = {};
		bool optimize;

#ifdef NDEBUG
		optimize = true;
#else
		optimize = false;
#endif
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_size);
		options.SetForcedVersionProfile(450, shaderc_profile::shaderc_profile_none);
		
		shaderc::SpvCompilationResult vertModule = compiler.CompileGlslToSpv(vertSrc, shaderc_shader_kind::shaderc_glsl_vertex_shader, (m_Name + ".vert").c_str(), options);
		if (vertModule.GetCompilationStatus() != shaderc_compilation_status_success)
			std::cerr << "Shader " << m_Name << " failed to compile: " << vertModule.GetErrorMessage() << std::endl;

		shaderc::SpvCompilationResult fragModule = compiler.CompileGlslToSpv(fragSrc, shaderc_shader_kind::shaderc_fragment_shader, (m_Name + ".frag").c_str(), options);
		if (fragModule.GetCompilationStatus() != shaderc_compilation_status_success)
			std::cerr << "Shader " << m_Name << " failed to compile: " << vertModule.GetErrorMessage() << std::endl;

		vertBin = { vertModule.cbegin(), vertModule.cend() };
		fragBin = { fragModule.cbegin(), fragModule.cend() };
	}

	void Shader::CreateVkShader(const std::vector<uint32_t>& vertSpirv, const std::vector<uint32_t>& fragSpirv)
	{
		VkShaderModuleCreateInfo vertShaderInfo = {};
		vertShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertShaderInfo.codeSize = vertSpirv.size() * sizeof(uint32_t);
		vertShaderInfo.pCode = vertSpirv.data();

		VkShaderModuleCreateInfo fragShaderInfo = {};
		fragShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragShaderInfo.codeSize = fragSpirv.size() * sizeof(uint32_t);
		fragShaderInfo.pCode = fragSpirv.data();

		if (vkCreateShaderModule(m_Device, &vertShaderInfo, nullptr, &m_VertModule) != VK_SUCCESS)
			std::cout << "Failed creating vertex shader" << std::endl;
		if (vkCreateShaderModule(m_Device, &fragShaderInfo, nullptr, &m_FragModule) != VK_SUCCESS)
			std::cout << "Failed creating fragment shader" << std::endl;

	}
}