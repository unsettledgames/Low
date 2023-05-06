#pragma once

namespace Low
{
	class Shader;
	class DescriptorSetLayout;
	class RenderPass;

	struct PushConsts
	{
		glm::vec3 CameraPos;
		float Metallic;
		float Roughness;
		float AO;
	};

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline(Ref<Shader> shader, const DescriptorSetLayout& descLayout, Ref<RenderPass> renderPass, const glm::vec2& size);
		~GraphicsPipeline();

		void Bind();

		inline operator VkPipeline() { return m_Handle; }
		inline VkPipelineLayout Layout() { return m_Layout; }

	private:
		VkPipeline m_Handle;
		VkPipelineLayout m_Layout;
	};
}