#pragma once

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

namespace Low
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		glm::vec3 Normal;

		static VkVertexInputBindingDescription GetVertexBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions()
		{
			std::vector<VkVertexInputAttributeDescription> ret(4);

			ret[0].binding = 0;
			ret[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			ret[0].offset = 0;
			ret[0].location = 0;

			ret[1].binding = 0;
			ret[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			ret[1].offset = sizeof(glm::vec3);
			ret[1].location = 1;

			ret[2].binding = 0;
			ret[2].format = VK_FORMAT_R32G32_SFLOAT;
			ret[2].offset = sizeof(glm::vec4) + sizeof(glm::vec3);
			ret[2].location = 2;

			ret[3].binding = 0;
			ret[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			ret[3].offset = sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(glm::vec2);
			ret[3].location = 3;

			return ret;
		}

		bool operator ==(const Vertex& other) const
		{
			return Position == other.Position && Color == other.Color && TexCoord == other.TexCoord;
		}
	};
}

namespace std {
	template<> struct hash<Low::Vertex> {
		size_t operator()(Low::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.Position) ^
				(hash<glm::vec4>()(vertex.Color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}