#pragma once

#include <string>

struct GLFWwindow;

namespace Low
{
	class Renderer
	{
	public:
		static void Init(const char** extensions, uint32_t nExtensions, GLFWwindow* windowHandle);
		static void Destroy();
	};
}