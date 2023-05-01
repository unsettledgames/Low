#pragma once

namespace Low
{
	class CommandBuffer;

	class Queue
	{
	public:
		enum class QueueType { None = 0, Graphics, Present, Transfer };

		Queue(VkQueue handle, QueueType type) : m_Handle(handle), m_Type(type) {}

		void Submit(std::vector<Ref<CommandBuffer>> buf);
		void Present(Ref<CommandBuffer> buf);

		inline operator VkQueue() { return m_Handle; }

	private:
		VkQueue m_Handle;
		QueueType m_Type;
	};
}