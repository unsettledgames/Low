#include <Vulkan/Command/ImmediateCommands.h>
#include <Structures/Buffer.h>
#include <Vulkan/Queue.h>
#include <Vulkan/VulkanCore.h>

namespace Low
{
	struct OTCState
	{
		VkCommandPool CommandPool;
	} s_OTCState;

	void ImmediateCommands::CopyBuffer(VkBuffer dst, VkBuffer src, size_t size)
	{
		VkCommandBuffer commandBuffer = Begin();
		{
			VkBufferCopy copyRegion;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
		}
		End(commandBuffer);
	}

	void ImmediateCommands::CopyBufferToImage(VkImage dst, VkBuffer src, uint32_t width, uint32_t height)
	{
		VkCommandBuffer cmdBuf = Begin();
		{
			VkBufferImageCopy reg = {};

			reg.bufferOffset = 0;
			reg.bufferRowLength = 0;
			reg.bufferImageHeight = 0;

			reg.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			reg.imageSubresource.mipLevel = 0;
			reg.imageSubresource.layerCount = 1;
			reg.imageSubresource.baseArrayLayer = 0;

			reg.imageOffset = { 0,0,0 };
			reg.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(cmdBuf, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
		}
		End(cmdBuf);
	}

	void ImmediateCommands::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer cmdBuf = Begin();
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;

			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_NONE;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			vkCmdPipelineBarrier(cmdBuf, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		}
		End(cmdBuf);
	}

	VkCommandBuffer ImmediateCommands::Begin()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		VkCommandBuffer commandBuffer;

		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = s_OTCState.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(VulkanCore::Device(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Couldn't allocate command buffers");
		}
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Couldn't begin cmd buffer");

		return commandBuffer;
	}

	void ImmediateCommands::End(VkCommandBuffer cmdBuffer)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;

		vkQueueSubmit(*VulkanCore::GraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(*VulkanCore::GraphicsQueue());

		vkFreeCommandBuffers(VulkanCore::Device(), s_OTCState.CommandPool, 1, &cmdBuffer);
	}

	void ImmediateCommands::Init(VkCommandPool pool)
	{
		s_OTCState.CommandPool = pool;
	}
}