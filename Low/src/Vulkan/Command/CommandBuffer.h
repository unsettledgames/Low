#pragma once

namespace Low
{
	class CommandBuffer
	{
	public:
		CommandBuffer(VkCommandBuffer buf);

		void Reset();
		
		void Begin();
		void End();

		inline VkCommandBuffer Handle() { return m_Handle; }

	private:
		VkCommandBuffer m_Handle;
	};
}