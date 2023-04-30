#pragma once

namespace Low
{
	class CommandBuffer;
	class GraphicsPipeline;
	class Framebuffer;

	class State
	{
	public:
		static void BindCommandBuffer(Ref<CommandBuffer> buffer);
		static void BindPipeline(Ref<GraphicsPipeline> pipeline);
		static void SetFramebuffer(Ref<Framebuffer> buffer);

		static Ref<CommandBuffer> CommandBuffer();
		static Ref<GraphicsPipeline> Pipeline();
		static Ref<Framebuffer> Framebuffer();

		static uint32_t CurrentImageIndex();
		static uint32_t CurrentFramebufferIndex();
	
	private:

	};
}