#include <Vulkan/Command/CommandPool.h>
#include <Hardware/Support.h>
#include <Vulkan/VulkanCore.h>

#include <Vulkan/Command/CommandBuffer.h>

namespace Low
{
	CommandPool::CommandPool(const QueueFamilyIndices& queueIndices)
	{
		VkCommandPoolCreateInfo commandPoolInfo = {};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = queueIndices.Graphics.value();

		if (vkCreateCommandPool(VulkanCore::Device(), &commandPoolInfo, nullptr, &m_Handle) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create command pool");
	}

	std::vector<Ref<CommandBuffer>> CommandPool::AllocateCommandBuffers(uint32_t count)
	{
		std::vector<Ref<CommandBuffer>> ret;
		std::vector<VkCommandBuffer> buffers;
		
		ret.resize(count);
		buffers.resize(count);
		
		VkCommandBufferAllocateInfo allocInfo{};

		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_Handle;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = count;

		if (vkAllocateCommandBuffers(VulkanCore::Device(), &allocInfo, buffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Couldn't allocate command buffers");
		}

		for (uint32_t i = 0; i < buffers.size(); i++)
			ret[i] = CreateRef<CommandBuffer>(buffers[i]);

		return ret;
	}
}