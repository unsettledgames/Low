#include <Vulkan/Queue.h>
#include <Core/State.h>

namespace Low
{
	void Queue::Submit(Ref<CommandBuffer> cmdBuffer)
	{
		/*
		assert(m_Type == QueueType::Graphics);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &s_Synch[s_State.CurrentFrame].SemImageAvailable;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &s_Data.CommandBuffers[s_State.CurrentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &s_Synch[s_State.CurrentFrame].SemRenderFinished;

		if (vkQueueSubmit(*s_Data.GraphicsQueue, 1, &submitInfo, s_Synch[s_State.CurrentFrame].FenInFlight) != VK_SUCCESS)
			throw std::runtime_error("Couldn't submit queue for rendering");
			*/
	}

	void Queue::Present(Ref<CommandBuffer> cmdBuffer)
	{
		assert(m_Type == QueueType::Present);
	}
}