#pragma once

namespace Low
{
	struct QueueFamilyIndices;

	class CommandBuffer;

	class CommandPool
	{
	public:
		CommandPool(const QueueFamilyIndices& queueIndices);
		~CommandPool();

		std::vector<Ref<CommandBuffer>> AllocateCommandBuffers(uint32_t count);

		inline operator VkCommandPool() { return m_Handle; }

	private:
		VkCommandPool m_Handle;
	};
}