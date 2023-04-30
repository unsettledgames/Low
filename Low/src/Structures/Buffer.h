#pragma once

#include <glm/glm.hpp>

namespace Low
{
	enum class BufferUsage {TransferSrc = 0, TransferDst, Vertex, Index, Uniform };

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(uint32_t size, BufferUsage usage);
		Buffer(uint32_t size, void* data, BufferUsage usage);

		~Buffer();
		
		inline VkDeviceMemory Memory() { return m_Memory; }

		inline size_t Size() { return m_Size; }
		void SetData(void* data);

		inline operator VkBuffer() { return m_Handle; }

	private:
		void Init(uint32_t size, BufferUsage usage);

	private:
		VkBuffer m_Handle;
		VkDeviceMemory m_Memory;

		size_t m_Size;
	};
}