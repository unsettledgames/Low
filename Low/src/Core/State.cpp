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

		uint32_t CurrentImageIndex;
		uint32_t CurrentFramebufferIndex;
	} s_CoreState;

	void State::BindCommandBuffer(Ref<Low::CommandBuffer> buf) { s_CoreState.CommandBuffer = buf; }
	void State::BindPipeline(Ref<GraphicsPipeline> pipeline) { s_CoreState.Pipeline = pipeline; }

	void State::SetCurrentFrameIndex(uint32_t index) { s_CoreState.CurrentFramebufferIndex = index; }
	void State::SetCurrentImageIndex(uint32_t index) { s_CoreState.CurrentImageIndex = index; }
	void State::SetFramebuffer(Ref<Low::Framebuffer> fb) { s_CoreState.Framebuffer = fb; }

	Ref<Framebuffer> State::Framebuffer() { return s_CoreState.Framebuffer; }
	Ref<CommandBuffer> State::CommandBuffer() { return s_CoreState.CommandBuffer; }
	Ref<GraphicsPipeline> State::Pipeline() { return s_CoreState.Pipeline; }

	uint32_t State::CurrentFramebufferIndex() { return s_CoreState.CurrentFramebufferIndex; }
	uint32_t State::CurrentImageIndex() { return s_CoreState.CurrentImageIndex; }
}