#pragma once

namespace Low
{
	class Semaphore
	{
	public:
		Semaphore(uint32_t framesInFlight);
		Semaphore() = default;
		Semaphore(const Semaphore& other) = default;
		~Semaphore();

		void Wait();
		void Signal();

		operator VkSemaphore();

	private:
		std::vector<VkSemaphore> m_Handles;
	};

	class Fence
	{
	public:
		Fence(uint32_t framesInFlight);
		Fence() = default;
		Fence(const Fence& other) = default;
		~Fence();

		void Wait();
		void Signal();

		operator VkFence();

	private:
		std::vector<VkFence> m_Handles;
	};

	class Synchronization
	{
	public:
		static void Init(uint32_t framesInFlight);

		static Ref<Semaphore> CreateSemaphore(const std::string& name);
		static Ref<Fence> CreateFence(const std::string& name);

		static inline void DeleteSemaphore(const std::string& name);
		static inline void DeleteFence(const std::string& name);

		static inline Ref<Semaphore> GetSemaphore(const std::string& name) 
		{ 
			if (s_Semaphores.find(name) != s_Semaphores.end())
				return WrapRef<Semaphore>(&s_Semaphores[name]);
			return nullptr;
		}

		static inline Ref<Fence> GetFence(const std::string& name)
		{
			if (s_Fences.find(name) != s_Fences.end())
				return WrapRef<Fence>(&s_Fences[name]);
			return nullptr;
		}
	
	private:
		static std::unordered_map<std::string, Semaphore> s_Semaphores;
		static std::unordered_map<std::string, Fence> s_Fences;
		static uint32_t s_FramesInFlight;
	};
}