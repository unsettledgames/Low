#include <Resources/Mesh.h>

#include <Vulkan/VulkanCore.h>

#include <Structures/Buffer.h>
#include <Structures/Vertex.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace Low
{
	Mesh::Mesh(const std::string& path)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
			throw std::runtime_error(warn + err);
		}

		for (const auto& shape : shapes)
		{
			for (const auto& idx : shape.mesh.indices)
			{
				Vertex v = {};

				v.Position = {
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2]
				};

				v.TexCoord = {
					attrib.texcoords[2 * idx.texcoord_index + 0],
					attrib.texcoords[2 * idx.texcoord_index + 1],
				};

				v.Color = glm::vec4(1.0f);

				vertices.push_back(v);
				indices.push_back(indices.size());
			}
		}

		m_VertexBuffer = CreateRef<Buffer>(vertices.size() * sizeof(Vertex), vertices.data(), BufferUsage::Vertex);
		m_IndexBuffer = CreateRef<Buffer>(indices.size() * sizeof(uint32_t), indices.data(), BufferUsage::Index);
	}
}