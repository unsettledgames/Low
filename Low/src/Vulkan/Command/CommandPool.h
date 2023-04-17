#pragma once

namespace Low
{
	struct QueueFamilyIndices;

	class CommandBuffer;

	class CommandPool
	{
	public:
		CommandPool(const QueueFamilyIndices& queueIndices);

		std::vector<Ref<CommandBuffer>> AllocateCommandBuffers(uint32_t count);

		inline VkCommandPool Handle() { return m_Handle; }

	private:
		VkCommandPool m_Handle;
	};
}