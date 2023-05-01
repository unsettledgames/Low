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


	Semaphore::~Semaphore()
	{
		for (auto& sem : m_Handles)
			vkDestroySemaphore(VulkanCore::Device(), sem, nullptr);
	}

	Semaphore::operator VkSemaphore() { return m_Handles[State::CurrentFramebufferIndex()]; }

	/********************************************************FENCE**********************************************************/

	Fence::Fence(uint32_t framesInFlight)
	{
		m_Handles.resize(framesInFlight);

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < framesInFlight; i++)
			vkCreateFence(VulkanCore::Device(), &fenceInfo, nullptr, &m_Handles[i]);
	}

	Fence::~Fence()
	{
		for (auto& fen : m_Handles)
			vkDestroyFence(VulkanCore::Device(), fen, nullptr);
	}

	Fence::operator VkFence() { return m_Handles[State::CurrentFramebufferIndex()]; }

	/********************************************************BARRIER?**********************************************************/

	/********************************************************SYNCHRONIZATION*********************************************************/

	std::unordered_map<std::string, Ref<Semaphore>> Synchronization::s_Semaphores;
	std::unordered_map<std::string, Ref<Fence>> Synchronization::s_Fences;
	uint32_t Synchronization::s_FramesInFlight;

	void Synchronization::Init(uint32_t inFlight)
	{
		s_FramesInFlight = inFlight;
	}

	Ref<Semaphore> Synchronization::CreateSemaphore(const std::string& name)
	{
		s_Semaphores[name] = CreateRef<Semaphore>(s_FramesInFlight);
		return s_Semaphores[name];
	}

	Ref<Fence> Synchronization::CreateFence(const std::string& name)
	{
		s_Fences[name] = CreateRef<Fence>(s_FramesInFlight);
		return s_Fences[name];
	}
}