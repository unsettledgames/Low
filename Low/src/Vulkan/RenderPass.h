#pragma once

namespace Low
{
	struct FramebufferAttachmentSpecs;

	class GraphicsPipeline;

	class RenderPass
	{
	public:
		RenderPass(const std::vector<FramebufferAttachmentSpecs>& specs);

		void Begin(Ref<GraphicsPipeline> pipeline, const glm::vec2& screenSize);
		void End();

		inline operator VkRenderPass() { return m_Handle; }

	private:
		VkRenderPass m_Handle;
	};
}