#pragma once

#include <string>

/*	TODO: struct to initialize the renderer. It should contain a working directory in which assets are stored, a Cache directory used to store intermediate 
	format stuff, extensions that should be enabled...
	
	TODO: maybe create a central creation hub? Something to hold the VkInstance and create stuff, something like a VulkanCore class
*/



struct GLFWwindow;

namespace Low
{
	class Renderer
	{
	public:
		static void Init(const char** extensions, uint32_t nExtensions, GLFWwindow* windowHandle);
		static void DrawFrame();
		static void Destroy();
	};
}