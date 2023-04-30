#include <Synchronization/Synchronization.h>
#include <Vulkan/VulkanCore.h>
#include <Core/State.h>

namespace Low
{
	/********************************************************SEMAPHORE**********************************************************/

	Semaphore::Semaphore(uint32_t framesInFlight)
	{
		m_Handles.resize(framesInFlight);

		VkSemaphoreCreateInfo semInfo = {};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (uint32_t i = 0; i < framesInFlight; i++)
			vkCreateSemaphore(VulkanCore::Device(), &semInfo, nullptr, &m_Handles[i]);
	}

	Semaphore::operator VkSemaphore() { return m_Handles[State::CurrentFramebufferIndex()]; }

	/********************************************************FENCE**********************************************************/

	Fence::Fence(uint32_t framesInFlight)
	{
		m_Handles.resize(framesInFlight);

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		for (uint32_t i = 0; i < framesInFlight; i++)
			vkCreateFence(VulkanCore::Device(), &fenceInfo, nullptr, &m_Handles[i]);
	}

	Fence::operator VkFence() { return m_Handles[State::CurrentFramebufferIndex()]; }

	/********************************************************BARRIER?**********************************************************/

	/********************************************************SYNCHRONIZATION*********************************************************/

	std::unordered_map<std::string, Semaphore> Synchronization::s_Semaphores;
	std::unordered_map<std::string, Fence> Synchronization::s_Fences;
	uint32_t Synchronization::s_FramesInFlight;

	void Synchronization::Init(uint32_t inFlight)
	{
		s_FramesInFlight = inFlight;
	}

	Ref<Semaphore> Synchronization::CreateSemaphore(const std::string& name)
	{
		s_Semaphores[name] = Semaphore(s_FramesInFlight);
		return WrapRef<Semaphore>(&s_Semaphores[name]);
	}

	Ref<Fence> Synchronization::CreateFence(const std::string& name)
	{
		s_Fences[name] = Fence(s_FramesInFlight);
		return WrapRef<Fence>(&s_Fences[name]);
	}
}