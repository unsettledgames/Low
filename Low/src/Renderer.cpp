#include <Renderer.h>

#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Core/Debug.h>
#include <Vulkan/VulkanCore.h>
#include <Vulkan/Swapchain.h>
#include <Vulkan/RenderPass.h>
#include <Vulkan/GraphicsPipeline.h>

#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/Descriptor/DescriptorPool.h>
#include <Vulkan/Descriptor/DescriptorSet.h>

#include <Vulkan/Command/CommandPool.h>
#include <Vulkan/Command/CommandBuffer.h>
#include <Vulkan/Command/OneTimeCommands.h>

#include <Hardware/Support.h>
#include <Hardware/Memory.h>

#include <Structures/Framebuffer.h>
#include <Structures/Buffer.h>
#include <Structures/Vertex.h>

#include <Resources/Texture.h>
#include <Resources/Shader.h>
#include <Resources/Mesh.h>

#include <GLFW/glfw3.h>
#include <stb_image.h>

// This stuff should probably go in a VulkanCore class or something. Then expose globals init and shutdown that initialize and shutdown
// everything. Instances, devices and stuff like that don't have much to do with the actual renderer

namespace Low
{
	static RendererConfig s_Config;

	struct RendererResources
	{
		// Buffers
		std::vector<Ref<Buffer>> UniformBuffers;

		// Resources
		Ref<Shader> Shader;
		Ref<Texture> Texture;
		Ref<Mesh> Mesh;
	};

	struct RendererData
	{
		// Device
		VkSurfaceKHR WindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		VkQueue PresentationQueue = VK_NULL_HANDLE;

		// Resources
		RendererResources* Resources;

		Ref<Low::Swapchain> SwapchainRef;
		VkExtent2D SwapchainExtent;

		// Pipeline
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;
		VkPipeline GraphicsPipeline;

		// Commands
		VkCommandPool CommandPool;
		std::vector<VkCommandBuffer> CommandBuffers;

		// Uniforms
		VkDescriptorSetLayout DescriptorSetLayout;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSet> DescriptorSets;
		
		std::vector<VkDeviceMemory> UniformBuffersMemory;
		std::vector<void*> UniformBuffersMapped;

		GLFWwindow* WindowHandle;
		std::vector<const char*> RequiredExtesions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::vector<Ref<Framebuffer>> Framebuffers;

	} s_Data;

	struct RendererState
	{
		uint32_t CurrentFrame = 0;
		bool FramebufferResized = false;
	} s_State;

	struct Synch
	{
		VkSemaphore SemImageAvailable;
		VkSemaphore SemRenderFinished;
		VkFence FenInFlight;
	};

	struct UniformBufferObject
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};

	std::vector<Synch> s_Synch;

	static VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (auto format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(VulkanCore::PhysicalDevice(), format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		throw std::runtime_error("Couldn't find a supported depth format");
	}

	static VkFormat FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	static void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = s_Data.RenderPass;
		renderPassInfo.framebuffer = s_Data.Framebuffers[imageIndex]->Handle();
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = s_Data.SwapchainExtent;

		std::array<VkClearValue, 2> clearColors;
		clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
		clearColors[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = clearColors.size();
		renderPassInfo.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			VkViewport viewport = {};
			VkRect2D scissors = {};

			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = s_Data.SwapchainExtent.width;
			viewport.height = s_Data.SwapchainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			scissors.extent = s_Data.SwapchainExtent;
			scissors.offset.x = 0.0f;
			scissors.offset.y = 0.0f;

			VkBuffer buffers[] = { s_Data.Resources->Mesh->VertexBuffer()->Handle()};
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.GraphicsPipeline);

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissors);

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, s_Data.Resources->Mesh->IndexBuffer()->Handle(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.PipelineLayout, 0, 1,
				&s_Data.DescriptorSets[s_State.CurrentFrame], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, s_Data.Resources->Mesh->IndexBuffer()->Size() / sizeof(uint32_t), 1, 0, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Error ending the command buffer");
	}

	static void CreateSynchronizationObjects()
	{
		s_Synch.resize(s_Config.MaxFramesInFlight);

		for (uint32_t i = 0; i < s_Synch.size(); i++)
		{
			VkSemaphoreCreateInfo semInfo = {};
			VkFenceCreateInfo fenInfo = {};

			semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateSemaphore(VulkanCore::Device(), &semInfo, nullptr, &s_Synch[i].SemImageAvailable) != VK_SUCCESS ||
				vkCreateSemaphore(VulkanCore::Device(), &semInfo, nullptr, &s_Synch[i].SemRenderFinished) != VK_SUCCESS ||
				vkCreateFence(VulkanCore::Device(), &fenInfo, nullptr, &s_Synch[i].FenInFlight) != VK_SUCCESS)
				throw std::runtime_error("Couldn't create synchronization structures");
		}
	}

	static void CleanupSwapchain()
	{
		/*
		for (auto& image : s_Data.SwapchainImageViews)
			vkDestroyImageView(VulkanCore::Device(), image, nullptr);
		for (auto& buf : s_Data.SwapchainFramebuffers)
			vkDestroyFramebuffer(VulkanCore::Device(), buf, nullptr);

		vkDestroyImage(VulkanCore::Device(), s_Data.DepthImage, nullptr);
		vkDestroyImageView(VulkanCore::Device(), s_Data.DepthImageView, nullptr);
		vkFreeMemory(VulkanCore::Device(), s_Data.DepthImageMemory, nullptr);

		vkDestroySwapchainKHR(VulkanCore::Device(), s_Data.Swapchain, nullptr);
		*/
	}

	static void RecreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(s_Data.WindowHandle, &width, &height);
		while (width == 0 || height == 0) 
		{
			glfwGetFramebufferSize(s_Data.WindowHandle, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(VulkanCore::Device());

		CleanupSwapchain();

		//CreateSwapchain();
		//CreateImageViews();
		//CreateDepthResources();
		//CreateFramebuffer();
	}

	static void OnFramebufferResize(GLFWwindow* window, int width, int height)
	{
		s_State.FramebufferResized = true;
	}

	static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(VulkanCore::PhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props)
				return i;
		}

		throw std::runtime_error("Couldn't find suitable memory type");
	}

	static void CreateUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		s_Data.Resources->UniformBuffers.resize(s_Config.MaxFramesInFlight);
		s_Data.UniformBuffersMemory.resize(s_Config.MaxFramesInFlight);
		s_Data.UniformBuffersMapped.resize(s_Config.MaxFramesInFlight);

		for (uint32_t i = 0; i < s_Config.MaxFramesInFlight; i++)
		{
			s_Data.Resources->UniformBuffers[i] = CreateRef<Buffer>(bufferSize, BufferUsage::Uniform);
			vkMapMemory(VulkanCore::Device(), s_Data.Resources->UniformBuffers[i]->Memory(), 0, bufferSize, 0, &s_Data.UniformBuffersMapped[i]);
		}
	}

	static void CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(s_Config.MaxFramesInFlight, s_Data.DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = s_Data.DescriptorPool;
		allocateInfo.descriptorSetCount = s_Config.MaxFramesInFlight;
		allocateInfo.pSetLayouts = layouts.data();

		s_Data.DescriptorSets.resize(s_Config.MaxFramesInFlight);
		if (vkAllocateDescriptorSets(VulkanCore::Device(), &allocateInfo, s_Data.DescriptorSets.data()) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate descriptor sets");

		for (uint32_t i = 0; i < s_Config.MaxFramesInFlight; i++)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = s_Data.Resources->UniformBuffers[i]->Handle();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo samplerInfo = {};
			samplerInfo.sampler = *s_Data.Resources->Texture->Sampler();
			samplerInfo.imageView = s_Data.Resources->Texture->ImageView();
			samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites({});

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = s_Data.DescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pNext = VK_NULL_HANDLE;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = s_Data.DescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &samplerInfo;

			vkUpdateDescriptorSets(VulkanCore::Device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	static void UpdateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		uint32_t w = s_Data.SwapchainExtent.width;
		uint32_t h = s_Data.SwapchainExtent.height;
		float ratio = (float)w / h;

		UniformBufferObject ubo = {};
		ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.View = glm::lookAt(glm::vec3(0.0f, 2, 2), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.Projection = glm::perspective(glm::radians(45.0f), (float)s_Data.SwapchainExtent.width / s_Data.SwapchainExtent.height, 0.1f, 10.0f);
		ubo.Projection[1][1] *= -1;

		memcpy(s_Data.UniformBuffersMapped[currentImage], &ubo, sizeof(UniformBufferObject));
	}
	
	static void CreateTextureImage()
	{
		s_Data.Resources->Texture = CreateRef<Texture>("../../Assets/Models/VikingRoom/viking_room.png", VulkanCore::Device(), VulkanCore::PhysicalDevice(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL);
		auto tex = s_Data.Resources->Texture;

		OneTimeCommands::TransitionImageLayout(tex->Handle(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		OneTimeCommands::CopyBufferToImage(tex->Handle(), tex->Buffer()->Handle(), tex->GetWidth(), tex->GetHeight());
		OneTimeCommands::TransitionImageLayout(tex->Handle(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	static void CreateTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = true;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(VulkanCore::PhysicalDevice(), &properties);

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(VulkanCore::Device(), &samplerInfo, nullptr, s_Data.Resources->Texture->Sampler()) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create sampler");

	}

	void Renderer::Init(RendererConfig config, GLFWwindow* windowHandle)
	{
		int width, height;
		glfwGetWindowSize(windowHandle, &width, &height);

		VulkanCoreConfig coreConfig;
		for (uint32_t i = 0; i < config.ExtensionCount; i++)
			coreConfig.UserExtensions.push_back(config.Extensions[i]);
		coreConfig.WindowHandle = windowHandle;

		s_Config = config;
		s_Data.WindowHandle = windowHandle;
		s_Data.Resources = new RendererResources();

		VulkanCore::Init(coreConfig);
		
		s_Data.WindowSurface = VulkanCore::Surface();
		s_Data.GraphicsQueue = VulkanCore::GraphicsQueue();
		s_Data.PresentationQueue = VulkanCore::PresentQueue();

		Ref<Swapchain> swapchain = CreateRef<Swapchain>(s_Data.WindowSurface, width, height);
		s_Data.SwapchainRef = swapchain;

		s_Data.Swapchain = swapchain->Handle();
		s_Data.SwapchainExtent = { (uint32_t)width, (uint32_t)height };

		glfwSetFramebufferSizeCallback(windowHandle, OnFramebufferResize);

		Ref<DescriptorSetLayout> descriptorSetLayout = CreateRef<DescriptorSetLayout>();
		s_Data.DescriptorSetLayout = descriptorSetLayout->Handle();

		Ref<DescriptorPool> descriptorPool = CreateRef<DescriptorPool>(s_Config.MaxFramesInFlight);
		s_Data.DescriptorPool = descriptorPool->Handle();

		CreateUniformBuffers();

		Ref<CommandPool> commandPool = CreateRef<CommandPool>(Support::GetQueueFamilyIndices(VulkanCore::PhysicalDevice(), s_Data.WindowSurface));
		s_Data.CommandPool = commandPool->Handle();

		OneTimeCommands::Init(commandPool->Handle());

		std::vector<Ref<CommandBuffer>> commandBuffers = commandPool->AllocateCommandBuffers(s_Config.MaxFramesInFlight);
		for (auto& buf : commandBuffers)
			s_Data.CommandBuffers.push_back(buf->Handle());

		std::vector<FramebufferAttachmentSpecs> attachmentSpecs = {
			{AttachmentType::Color, VK_FORMAT_B8G8R8A8_SRGB, 1, true},
			{AttachmentType::Depth, FindDepthFormat(), 1, false}
		};

		Ref<RenderPass> renderPass = CreateRef<RenderPass>(attachmentSpecs);
		s_Data.RenderPass = renderPass->Handle();

		for (uint32_t i = 0; i < s_Data.SwapchainRef->Images().size(); i++)
		{
			std::vector<VkImage> images;
			for (auto spec : attachmentSpecs)
				if (spec.IsSwapchain)
					images.push_back(swapchain->Images()[i]);
				else
					images.push_back(VK_NULL_HANDLE);

			Ref<Framebuffer> framebuffer = CreateRef<Framebuffer>(renderPass->Handle(), width, height, attachmentSpecs, images);
			s_Data.Framebuffers.push_back(framebuffer);
		}

		Ref<Shader> shader = CreateRef<Shader>("basic", VulkanCore::Device());
		s_Data.Resources->Shader = shader;
		Ref<GraphicsPipeline> graphicsPipeline = CreateRef<GraphicsPipeline>(shader, descriptorSetLayout, renderPass, glm::vec2(s_Data.SwapchainExtent.width, s_Data.SwapchainExtent.height));
		s_Data.GraphicsPipeline = graphicsPipeline->Handle();
		s_Data.PipelineLayout = graphicsPipeline->Layout();

		/* TODO:
		* - Easier way to handle format transfers 
		* - Easier way to create images
		* - Expose uniform memory
		* 
		* - Remove as much stuff from s_Data (iteratively)
		* 
		* - Abstract DrawFrame
		* - See if we can abstract stuff a bit more besides just copy pasting code in many classes (especially regarding uniforms)
		* - Resource destruction
		* - Implement basic API
		* 
		* - Start implementing deferred PBR rendering
		
		*/

		Ref<Mesh> mesh = CreateRef<Mesh>("../../Assets/Models/VikingRoom/viking_room.obj");
		s_Data.Resources->Mesh = mesh;
		
		CreateTextureImage();
		CreateTextureSampler();

		CreateDescriptorSets();

		CreateSynchronizationObjects();
	}
	

	void Renderer::DrawFrame()
	{
		/*
			STAGES:
			
			- Upload / Update Uniforms
			- Invalid swapchain if necessary
			- Submit queue
			- Present queue
		*/

		/*
			COMMAND BUFFER

			- Reset
			- Record:
				- Expose API
		*/

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		uint32_t imageIdx;

		UpdateUniformBuffer(s_State.CurrentFrame);
		vkWaitForFences(VulkanCore::Device(), 1, &s_Synch[s_State.CurrentFrame].FenInFlight, VK_TRUE, UINT64_MAX);

		VkResult res = vkAcquireNextImageKHR(VulkanCore::Device(), s_Data.Swapchain, UINT64_MAX, 
			s_Synch[s_State.CurrentFrame].SemImageAvailable, VK_NULL_HANDLE, &imageIdx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}

		vkResetFences(VulkanCore::Device(), 1, &s_Synch[s_State.CurrentFrame].FenInFlight);
		vkResetCommandBuffer(s_Data.CommandBuffers[s_State.CurrentFrame], 0);
		
		RecordCommandBuffer(s_Data.CommandBuffers[s_State.CurrentFrame], imageIdx);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &s_Synch[s_State.CurrentFrame].SemImageAvailable;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &s_Data.CommandBuffers[s_State.CurrentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &s_Synch[s_State.CurrentFrame].SemRenderFinished;

		if (vkQueueSubmit(s_Data.GraphicsQueue, 1, &submitInfo, s_Synch[s_State.CurrentFrame].FenInFlight) != VK_SUCCESS)
			throw std::runtime_error("Couldn't submit queue for rendering");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &s_Synch[s_State.CurrentFrame].SemRenderFinished;
		
		VkSwapchainKHR swapChains = { s_Data.Swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChains;
		presentInfo.pImageIndices = &imageIdx;
		presentInfo.pResults = nullptr;

		res = vkQueuePresentKHR(s_Data.PresentationQueue, &presentInfo);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || s_State.FramebufferResized)
		{
			RecreateSwapchain();
			s_State.FramebufferResized = false;
			return;
		}
		s_State.CurrentFrame = (s_State.CurrentFrame + 1) % s_Config.MaxFramesInFlight;
	}

	void Renderer::Destroy()
	{
		Debug::Shutdown();
		
		vkDeviceWaitIdle(VulkanCore::Device());

		CleanupSwapchain();

		vkDestroyPipelineLayout(VulkanCore::Device(), s_Data.PipelineLayout, nullptr);
		vkDestroyPipeline(VulkanCore::Device(), s_Data.GraphicsPipeline, nullptr);
		vkDestroyRenderPass(VulkanCore::Device(), s_Data.RenderPass, nullptr);

		vkDestroyCommandPool(VulkanCore::Device(), s_Data.CommandPool, nullptr);

		for (uint32_t i = 0; i < s_Synch.size(); i++)
		{
			vkDestroySemaphore(VulkanCore::Device(), s_Synch[i].SemImageAvailable, nullptr);
			vkDestroySemaphore(VulkanCore::Device(), s_Synch[i].SemRenderFinished, nullptr);
			vkDestroyFence(VulkanCore::Device(), s_Synch[i].FenInFlight, nullptr);
		}

		vkDestroyDescriptorPool(VulkanCore::Device(), s_Data.DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(VulkanCore::Device(), s_Data.DescriptorSetLayout, nullptr);

		delete s_Data.Resources;

	}
}