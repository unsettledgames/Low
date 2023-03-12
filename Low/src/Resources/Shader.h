#pragma once

#include <string>

namespace Low
{
	class Shader
	{
	public:
		Shader(const std::string& shaderName);

	private:
		void Compile(const std::string& vertSrc, const std::string& fragSrc);

	private:
		std::string m_Name;
	};
}