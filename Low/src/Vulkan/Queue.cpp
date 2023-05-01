#include <Vulkan/Queue.h>
#include <Vulkan/Command/CommandBuffer.h>
#include <Synchronization/Synchronization.h>
#include <Core/State.h>

namespace Low
{
	void Queue::Submit(std::vector<Ref<CommandBuffer>> cmdBuffers)
	{
		std::vector<VkCommandBuffer> vkBuffers(cmdBuffers.size());
		for (uint32_t i = 0; i < cmdBuffers.size(); i++)
			vkBuffers[i] = *cmdBuffers[i];

		assert(m_Type == QueueType::Graphics);

		VkSubmitInfo submitInfo = {};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		std::vector<VkSemaphore> waitSems = { *Synchronization::GetSemaphore("ImageAvailable") };
		std::vector<VkSemaphore> signalSems = { *Synchronization::GetSemaphore("RenderFinished") };

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = waitSems.size();
		submitInfo.pWaitSemaphores = waitSems.data();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = vkBuffers.size();
		submitInfo.pCommandBuffers = vkBuffers.data();
		submitInfo.signalSemaphoreCount = signalSems.size();
		submitInfo.pSignalSemaphores = signalSems.data();

		if (vkQueueSubmit(m_Handle, 1, &submitInfo, *Synchronization::GetFence("FrameInFlight")) != VK_SUCCESS)
			throw std::runtime_error("Couldn't submit queue for rendering");
	}

	void Queue::Present(Ref<CommandBuffer> cmdBuffer)
	{
		assert(m_Type == QueueType::Present);
		/*
		uint32_t currImage = State::CurrentImageIndex();
		VkSwapchainKHR swapChains = { s_Data.Swapchain };
		VkPresentInfoKHR presentInfo = {};

		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &s_Synch[State::CurrentFramebufferIndex()].SemRenderFinished;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChains;
		presentInfo.pImageIndices = &currImage;
		presentInfo.pResults = nullptr;

		res = vkQueuePresentKHR(*s_Data.PresentationQueue, &presentInfo);
		*/
	}
}