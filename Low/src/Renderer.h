#pragma once

/*
*	RENDERER BEGIN:
*		- PARAMETERS:
*			- Global uniforms, they'll be the same for each freaking object
*		- PARTS:
*			- Update / upload uniforms
*			- Get image to write on
* 
*	RENDERER MID:
*		- Submit command buffers
* 
*	RENDERER END:
*		- Submit queue
*		- Present
* 
* 
*	COMMAND BUFFER
*		- Begin
*		- Mid:
*			- Submit render passes
*		- End
* 
*	Render passes can be re-recorded every time or recorded once and then reused
*   Each render pass renders using its own pipeline.
* 
*	RENDER PASS
*		- Begin:
*			- Bind pipeline
*			- Set viewport / scissors
*			
*		- Mid:
*			- Submit commands
*			- For each model: 
*				- Update its uniforms
*				- Send models
*		- End
* 
* 
*  Sooo, the first thing I should work on is a single render pass. In that way I should probably also be able to send ImGui's data. 
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

		static void BeginRenderPass(VkCommandBuffer cmdBuffer);
		static void EndRenderPass(VkCommandBuffer cmdBuffer);

		static void DrawFrame();
		static void Destroy();
	};
}