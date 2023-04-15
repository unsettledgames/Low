#pragma once

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

namespace Low
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;

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
			std::vector<VkVertexInputAttributeDescription> ret(3);

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

			return ret;
		}
	};
}