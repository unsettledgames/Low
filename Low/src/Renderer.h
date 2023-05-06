#pragma once

/*
*	CURRENT:	integration of the changes written below to the DrawFrame() function.
* 
*	USER API:
*		- Begin: send global uniforms
*			- [VERTEX]		Camera view
*			- [VERTEX]		Camera projection
*			- [FRAGMENT]	Shadow maps
*			- [FRAGMENT]	Lights
* 
*		- Since we're at it, I could just start a better material system.

			- Frequently changing uniforms (push constants):
				- Transform matrix
			- Static-ish properties:
				- Scalar material properties
				- Textures
* 
*		- PushCommandBuffer: add a custom command buffer at the end
*		- EmplaceCommandBuffer: add a custom command buffer at the beginning
* 
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

	class MaterialInstance;
	class Mesh;
	class Camera;

	struct RendererConfig
	{
		const char** Extensions;
		uint32_t ExtensionCount;

		uint32_t MaxFramesInFlight;
	};

	struct Renderable
	{
		Ref<Mesh> Mesh;
		Ref<MaterialInstance> Material;
		glm::mat4 Transform;
	};

	class Renderer
	{
	public:
		static void Init(RendererConfig config, GLFWwindow* windowHandle);

		static void Begin(const Camera& camera);
		static void PushModel(Ref<Mesh> mesh, Ref<MaterialInstance> material, const glm::mat4& transform);
		static void End();

		static void DrawFrame();
		static void Destroy();

	private:
		static void Optimize();
		static void PrepareResources();

	private:
		static std::vector<Ref<CommandBuffer>> s_CommandBuffers;
		static std::unordered_map<UUID, std::vector<Renderable>> s_Renderables;
	};
}