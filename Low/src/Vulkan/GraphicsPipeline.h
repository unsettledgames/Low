#pragma once

namespace Low
{
	class Shader;
	class DescriptorSetLayout;
	class RenderPass;

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(Ref<Shader> shader, Ref<DescriptorSetLayout> descLayout, Ref<RenderPass> renderPass, const glm::vec2& size);

		inline VkPipeline Handle() { return m_Handle; }
		inline VkPipelineLayout Layout() { return m_Layout; }

	private:
		VkPipeline m_Handle;
		VkPipelineLayout m_Layout;
	};
}