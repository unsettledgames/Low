#pragma once

#include <string>

/*	TODO: struct to initialize the renderer. It should contain a working directory in which assets are stored, a Cache directory used to store intermediate 
	format stuff, extensions that should be enabled...
	
	TODO: maybe create a central creation hub? Something to hold the VkInstance and create stuff, something like a VulkanCore class
*/



struct GLFWwindow;

namespace Low
{
	struct RendererConfig
	{
		const char** Extensions;
		uint32_t ExtensionCount;

		uint32_t MaxFramesInFlight;
	};

	class Renderer
	{
	public:
		static void Init(RendererConfig config, GLFWwindow* windowHandle);
		static void DrawFrame();
		static void Destroy();
	};
}