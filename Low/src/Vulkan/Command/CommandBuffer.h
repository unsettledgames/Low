#pragma once

namespace Low
{
	class RenderPass;

	class CommandBuffer
	{
	public:
		CommandBuffer(VkCommandBuffer buf);

		void Reset();
		void Begin();
		void End();

		inline operator VkCommandBuffer() { return m_Handle; }

	private:
		VkCommandBuffer m_Handle;
	};
}