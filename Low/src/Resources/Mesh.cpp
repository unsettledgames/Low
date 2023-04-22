#include <Resources/Mesh.h>

#include <Vulkan/VulkanCore.h>

#include <Structures/Buffer.h>
#include <Structures/Vertex.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace Low
{
	Mesh::Mesh(const std::string& path, VkDevice logicalDevice, VkDevice physicalDevice)
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
					attrib.texcoords[2 * idx.vertex_index + 0],
					attrib.texcoords[2 * idx.vertex_index + 1],
				};

				v.Color = glm::vec4(1.0f);

				vertices.push_back(v);
				indices.push_back(indices.size());
			}
		}

		m_VertexBuffer = CreateRef<Buffer>(vertices.size() * sizeof(Vertex), BufferUsage::Vertex);
		m_IndexBuffer = CreateRef<Buffer>(indices.size() * sizeof(uint32_t), BufferUsage::Index);

		/*
		m_VertexBuffer = CreateRef<Buffer>(logicalDevice, physicalDevice, sizeof(Vertex) * vertices.size(),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_IndexBuffer = CreateRef<Buffer>(logicalDevice, physicalDevice, sizeof(uint32_t) * indices.size(),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Ref<Buffer> vertexStagingBuffer = CreateRef<Buffer>(logicalDevice, physicalDevice, sizeof(Vertex) * vertices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		Ref<Buffer> indexStagingBuffer = CreateRef<Buffer>(logicalDevice, physicalDevice, sizeof(uint32_t) * indices.size(),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			*/
	}
}