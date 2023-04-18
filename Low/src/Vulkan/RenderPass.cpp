#include <Vulkan/RenderPass.h>
#include <Vulkan/VulkanCore.h>

#include <Structures/Framebuffer.h>

namespace Low
{
	RenderPass::RenderPass(const std::vector<FramebufferAttachmentSpecs>& specs)
	{
		std::vector<VkAttachmentDescription> descs;
		std::vector<VkAttachmentReference> refs;

		for (uint32_t i = 0; i < specs.size(); i++)
		{
			descs.push_back(specs[i].Description);
			refs.push_back(specs[i].Reference);
		}

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pResolveAttachments = nullptr;
		// The index is the index in the glsl shader layout!
		subpass.pColorAttachments = refs.data();
		for (uint32_t i=0; i<refs.size(); i++)
			if (refs[i].layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				subpass.pDepthStencilAttachment = &refs[i];
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = descs.size();
		renderPassInfo.pAttachments = descs.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dep = {};
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass = 0;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep.srcAccessMask = 0;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dep;

		if (vkCreateRenderPass(VulkanCore::Device(), &renderPassInfo, nullptr, &m_Handle))
			throw std::runtime_error("Failed to create render pass");
	}
}