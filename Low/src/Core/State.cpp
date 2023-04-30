#include <Vulkan/Command/CommandBuffer.h>
#include <Vulkan/GraphicsPipeline.h>
#include <Structures/Framebuffer.h>

#include <Core/State.h>

namespace Low
{
	struct CoreState
	{
		Ref<CommandBuffer> CommandBuffer;
		Ref<GraphicsPipeline> Pipeline;
		Ref<Framebuffer> Framebuffer;
	} s_CoreState;

	void State::BindCommandBuffer(Ref<Low::CommandBuffer> buf) { s_CoreState.CommandBuffer = buf; }
	void State::BindPipeline(Ref<GraphicsPipeline> pipeline) { s_CoreState.Pipeline = pipeline; }
	void State::SetFramebuffer(Ref<Low::Framebuffer> buf) { s_CoreState.Framebuffer = buf; }

	Ref<CommandBuffer> State::CommandBuffer() { return s_CoreState.CommandBuffer; }
	Ref<GraphicsPipeline> State::Pipeline() { return s_CoreState.Pipeline; }
	Ref<Framebuffer> State::Framebuffer() { return s_CoreState.Framebuffer; }
}