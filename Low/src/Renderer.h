#pragma once

/*
*	CURRENT:	integration of the changes written below to the DrawFrame() function.
* 
*	USER API:
*		- Begin: send global uniforms
*		- PushModel: send a mesh, a material and its scene data
*		- PushCommandBuffer: add a custom command buffer at the end
*		- EmplaceCommandBuffer: add a custom command buffer at the beginning
*		- End: draw everything
* 
*	RENDERER BEGIN:
*		- PARAMETERS:
*			- Global uniforms, they'll be the same for each freaking object
*		- PARTS:
*			- Update / upload uniforms: https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
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
* MAIN ARCHITECTURE:
*		- Build the right pipelines (before starting, maybe have the users provide the shaders that will be used)
*		- Let users specify what they want to render
*			- Geometry
*			- Material
*			- Scene data for culling & other optimizations
*		
*		- Remove as much stuff to be rendered as possible
		- Group the models by material
		- Build the "right" render passes and command buffers. Each step could be a separate command buffer, that could be recorded beforehand
		- The idea is to minimize pipeline bindings and take advantage of the fact that most of the scene should stay the same and only a small
			subset of stuff changes over time, and even in that case, it's probably just the material / uniforms
*/

struct GLFWwindow;

namespace Low
{
	class CommandBuffer;

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

		static void Begin();
		static inline void PushCommandBuffer(Ref<CommandBuffer> cmdBuffer) { s_CommandBuffers.push_back(cmdBuffer); }
		static void End();

		static void BeginRenderPass(VkCommandBuffer cmdBuffer);
		static void EndRenderPass(VkCommandBuffer cmdBuffer);

		static void DrawFrame();
		static void Destroy();

	private:
		static std::vector<Ref<CommandBuffer>> s_CommandBuffers;
	};
}