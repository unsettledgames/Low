#include <Resources/Shader.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <shaderc/shaderc.hpp>

// https://gist.github.com/sortofsleepy/a7fcb8221f70cab37e82f3779e78aaa5

namespace Low
{
	Shader::Shader(const std::string& shaderName) : m_Name(shaderName)
	{
		std::string vertPath = "../Assets/Shaders/" + shaderName + ".vert";
		std::string fragPath = "../Assets/Shaders/" + shaderName + ".frag";

		std::ifstream vertFile(vertPath), fragFile(fragPath);
		if (!vertFile.is_open())
			throw std::runtime_error("Coudln't find vertex shader file");
		if (!fragFile.is_open())
			throw std::runtime_error("Coudln't find fragment shader file");

		// Get shader source
		std::string vertSource, fragSource;
		std::stringstream ss;
		ss >> vertFile.rdbuf();
		vertSource = ss.str();

		ss.str("");
		ss >> fragFile.rdbuf();
		fragSource = ss.str();

		// Compile and link
		Compile(vertSource, fragSource);
	}

	void Shader::Compile(const std::string& vertSrc, const std::string& fragSrc)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		bool optimize;

#ifdef NDEBUG
		optimize = true;
#else
		optimize = false;
#endif
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_size);

		shaderc::SpvCompilationResult vertModule = compiler.CompileGlslToSpv(vertSrc, shaderc_shader_kind::shaderc_glsl_vertex_shader,
			m_Name.c_str(), options);
		if (vertModule.GetCompilationStatus() != shaderc_compilation_status_success)
			std::cerr << "Shader " << m_Name << " failed to compile: " << vertModule.GetErrorMessage() << std::endl;
		
		std::vector<uint32_t> spirvData = { vertModule.cbegin(), vertModule.cend() };

	}
}