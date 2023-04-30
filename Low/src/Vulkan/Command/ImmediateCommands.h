#pragma once

namespace Low
{
	class Buffer;

	class ImmediateCommands
	{
	public:
		static void Init(VkCommandPool pool);

		static void CopyBuffer(VkBuffer dst, VkBuffer src, size_t size);
		static void CopyBufferToImage(VkImage dst, VkBuffer src, uint32_t imgWidth, uint32_t imgHeight);

		static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		static VkCommandBuffer Begin();
		static void End(VkCommandBuffer cmd);
	};
}