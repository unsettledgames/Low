#pragma once

namespace Low
{
	class Buffer;

	class Mesh
	{
	public:
		Mesh(const std::string& path, VkDevice logicalDevice, VkDevice physicalDevice);

	private:
		Ref<Buffer> m_VertexBuffer;
		Ref<Buffer> m_IndexBuffer;
	};
}