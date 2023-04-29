#include <Vulkan/Command/CommandBuffer.h>

namespace Low
{
	CommandBuffer::CommandBuffer(VkCommandBuffer buf) : m_Handle(buf) {}

	void CommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(m_Handle, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}
	}

	void CommandBuffer::End()
	{
		if (vkEndCommandBuffer(m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Error ending the command buffer");
	}
}