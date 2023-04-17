#pragma once

namespace Low
{
	class CommandBuffer
	{
	public:
		CommandBuffer(VkCommandBuffer buf);

		inline VkCommandBuffer Handle() { return m_Handle; }

	private:
		VkCommandBuffer m_Handle;
	};
}