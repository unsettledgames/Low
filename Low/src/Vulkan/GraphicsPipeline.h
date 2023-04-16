#pragma once

namespace Low
{
	class Shader;

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(Ref<Shader> shader, const glm::vec2& size);

		inline VkGraphicsPipeline Handle() { return m_Handle; }

	private:
		VkGraphicsPipeline m_Handle;
		VkGraphicsPipelineLayout m_Layout;
	};
}