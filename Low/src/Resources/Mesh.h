#pragma once

namespace Low
{
	class Buffer;

	class Mesh
	{
	public:
		Mesh(const std::string& path);

		inline Ref<Buffer> VertexBuffer() { return m_VertexBuffer; }
		inline Ref<Buffer> IndexBuffer() { return m_IndexBuffer; }

	private:
		Ref<Buffer> m_VertexBuffer;
		Ref<Buffer> m_IndexBuffer;
	};
}