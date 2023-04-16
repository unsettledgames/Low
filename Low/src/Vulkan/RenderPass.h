#pragma once

namespace Low
{
	class RenderPass
	{
	public:
		RenderPass(VkFormat colorFormat, VkFormat depthFormat);

		inline VkRenderPass Handle() { return m_Handle; }

	private:
		VkRenderPass m_Handle;
	};
}