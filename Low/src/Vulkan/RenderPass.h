#pragma once

namespace Low
{
	struct FramebufferAttachmentSpecs;

	class RenderPass
	{
	public:
		RenderPass(const std::vector<FramebufferAttachmentSpecs>& specs);

		inline VkRenderPass Handle() { return m_Handle; }

	private:
		VkRenderPass m_Handle;
	};
}