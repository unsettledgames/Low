#include <Renderer.h>

#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Core/Debug.h>
#include <Core/State.h>
#include <Synchronization/Synchronization.h>

#include <Vulkan/VulkanCore.h>
#include <Vulkan/Queue.h>
#include <Vulkan/Swapchain.h>
#include <Vulkan/RenderPass.h>
#include <Vulkan/GraphicsPipeline.h>

#include <Vulkan/Descriptor/DescriptorSetLayout.h>
#include <Vulkan/Descriptor/DescriptorPool.h>
#include <Vulkan/Descriptor/DescriptorSet.h>

#include <Vulkan/Command/CommandPool.h>
#include <Vulkan/Command/CommandBuffer.h>
#include <Vulkan/Command/ImmediateCommands.h>

#include <Hardware/Support.h>
#include <Hardware/Memory.h>

#include <Structures/Framebuffer.h>
#include <Structures/Buffer.h>
#include <Structures/Vertex.h>

#include <Resources/Texture.h>
#include <Resources/Shader.h>
#include <Resources/Mesh.h>
#include <Resources/MaterialInstance.h>

#include <GLFW/glfw3.h>
#include <stb_image.h>

namespace Low
{
	std::vector<Ref<CommandBuffer>> Renderer::s_CommandBuffers;
	std::unordered_map<UUID, std::vector<Renderable>> Renderer::s_Renderables;
	static RendererConfig s_Config;

	struct RendererResources
	{
		// Buffers
		std::vector<Ref<Buffer>> UniformBuffers;

		// Resources
		Ref<Shader> Shader;
		Ref<Low::Texture> Texture;
		Ref<Low::Texture> Roughness;
		Ref<Mesh> Mesh;
	};

	struct RendererData
	{
		Ref<Swapchain> Swapchain = VK_NULL_HANDLE;
		Ref<Queue> GraphicsQueue = VK_NULL_HANDLE;
		Ref<Queue> PresentationQueue = VK_NULL_HANDLE;

		// Resources
		RendererResources* Resources;

		// Pipeline
		Ref<RenderPass> RenderPass;
		Ref<GraphicsPipeline> GraphicsPipeline;
		std::vector<Ref<Framebuffer>> Framebuffers;

		// Commands
		Ref<CommandPool> CommandPool;
		std::vector<Ref<CommandBuffer>> CommandBuffers;

		// Uniforms
		VkDescriptorSetLayout DescriptorSetLayout;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSet> DescriptorSets;
		
		std::vector<VkDeviceMemory> UniformBuffersMemory;
		std::vector<void*> UniformBuffersMapped;

		GLFWwindow* WindowHandle;

	} s_Data;

	struct UniformBufferObject
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Projection;
	};

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

		std::vector<FramebufferAttachmentSpecs> attachmentSpecs = {
			{AttachmentType::Color, VK_FORMAT_B8G8R8A8_SRGB, 1, true},
			{AttachmentType::Depth, VK_FORMAT_D32_SFLOAT, 1, false}
		};

		s_Data.Swapchain->Invalidate(width, height);
		for (uint32_t i = 0; i < s_Data.Framebuffers.size(); i++)
		{
			std::vector<VkImage> images;
			for (auto spec : attachmentSpecs)
				if (spec.IsSwapchain)
					images.push_back(s_Data.Swapchain->Images()[i]);
				else
					images.push_back(VK_NULL_HANDLE);

			s_Data.Framebuffers[i]->Invalidate(*s_Data.RenderPass, width, height, attachmentSpecs, images);
		}
	}

	static void OnFramebufferResize(GLFWwindow* window, int width, int height)
	{
		RecreateSwapchain();
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
			bufferInfo.buffer = *s_Data.Resources->UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo roughness = {};
			roughness.sampler = *s_Data.Resources->Roughness->Sampler();
			roughness.imageView = s_Data.Resources->Roughness->ImageView();
			roughness.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo samplerInfo = {};
			samplerInfo.sampler = *s_Data.Resources->Texture->Sampler();
			samplerInfo.imageView = s_Data.Resources->Texture->ImageView();
			samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::array<VkWriteDescriptorSet, 3> descriptorWrites({});

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

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = s_Data.DescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &roughness;

			vkUpdateDescriptorSets(VulkanCore::Device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	static void UpdateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		uint32_t w = s_Data.Swapchain->Extent().x;
		uint32_t h = s_Data.Swapchain->Extent().y;
		float ratio = (float)w / h;

		UniformBufferObject ubo = {};
		ubo.Model = glm::rotate(glm::mat4(1.0f), time * 0.75f * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 4), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.Projection = glm::perspective(glm::radians(45.0f), (float)w / h, 0.1f, 10.0f);
		ubo.Projection[1][1] *= -1;

		memcpy(s_Data.UniformBuffersMapped[currentImage], &ubo, sizeof(UniformBufferObject));
	}
	
	static void CreateTextures()
	{
		s_Data.Resources->Texture = CreateRef<Texture>("../../Assets/Models/Sphere/Rusty/rustediron2_basecolor.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL);
		s_Data.Resources->Roughness = CreateRef<Texture>("../../Assets/Models/Sphere/Rusty/rustediron2_metallic.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL);
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
		
		s_Data.GraphicsQueue = VulkanCore::GraphicsQueue();
		s_Data.PresentationQueue = VulkanCore::PresentQueue();

		s_Data.Swapchain = CreateRef<Swapchain>(width, height);

		glfwSetFramebufferSizeCallback(windowHandle, OnFramebufferResize);

		Ref<DescriptorSetLayout> descriptorSetLayout = CreateRef<DescriptorSetLayout>();
		s_Data.DescriptorSetLayout = *descriptorSetLayout;

		Ref<DescriptorPool> descriptorPool = CreateRef<DescriptorPool>(s_Config.MaxFramesInFlight);
		s_Data.DescriptorPool = *descriptorPool;

		CreateUniformBuffers();

		s_Data.CommandPool = CreateRef<CommandPool>(Support::GetQueueFamilyIndices(VulkanCore::PhysicalDevice(), VulkanCore::Surface()));
		ImmediateCommands::Init(*s_Data.CommandPool);

		std::vector<Ref<CommandBuffer>> commandBuffers = s_Data.CommandPool->AllocateCommandBuffers(s_Config.MaxFramesInFlight);
		for (auto& buf : commandBuffers)
			s_Data.CommandBuffers.push_back(buf);

		std::vector<FramebufferAttachmentSpecs> attachmentSpecs = {
			{AttachmentType::Color, VK_FORMAT_B8G8R8A8_SRGB, 1, true},
			{AttachmentType::Depth, VK_FORMAT_D32_SFLOAT, 1, false}
		};

		s_Data.RenderPass = CreateRef<RenderPass>(attachmentSpecs);

		for (uint32_t i = 0; i < s_Data.Swapchain->Images().size(); i++)
		{
			std::vector<VkImage> images;
			for (auto spec : attachmentSpecs)
				if (spec.IsSwapchain)
					images.push_back(s_Data.Swapchain->Images()[i]);
				else
					images.push_back(VK_NULL_HANDLE);

			Ref<Framebuffer> framebuffer = CreateRef<Framebuffer>(*s_Data.RenderPass, width, height, attachmentSpecs, images);
			s_Data.Framebuffers.push_back(framebuffer);
		}

		Ref<Shader> shader = CreateRef<Shader>("basic");
		s_Data.Resources->Shader = shader;

		Ref<GraphicsPipeline> graphicsPipeline = CreateRef<GraphicsPipeline>(shader, descriptorSetLayout, s_Data.RenderPass, s_Data.Swapchain->Extent());
		s_Data.GraphicsPipeline = graphicsPipeline;

		/* TODO:
		* - Expose uniform memory
		* - Remove as much stuff from s_Data (iteratively)
		* 
		* - Abstract DrawFrame
		* - Implement basic API
		* 
		* - Start implementing deferred PBR rendering
		*	- Have a look at what PBR rendering needs. Each parameter is probably a framebuffer in the GBuffer
		*	- Store stuff in the G-Buffer:
		*		- Positions
		*		- Normals
		*		- Color
		*	- Do a depth only pass
		*	- Fill the G-buffer: the vertex shader returns the necessary data to the fragment shader, which just outputs to the right attachments
		*	- Draw the fullscreen quad: for each fragment, sample the previously filled attachments and do the necessary computations
		
		*/

		Ref<Mesh> mesh = CreateRef<Mesh>("../../Assets/Models/Sphere/sphere.obj");
		s_Data.Resources->Mesh = mesh;
		
		CreateTextures();
		CreateDescriptorSets();

		// Init Synchronization
		{
			Synchronization::Init(s_Config.MaxFramesInFlight);
			Synchronization::CreateSemaphore("ImageAvailable");
			Synchronization::CreateSemaphore("RenderFinished");
			Synchronization::CreateFence("FrameInFlight");
		}

		// Init state
		{
			State::SetCurrentFrameIndex(0);
			State::SetFramebuffer(s_Data.Framebuffers[0]);
		}
	}

	void Renderer::Begin()
	{

	}

	void Renderer::PushModel(Ref<Mesh> mesh, Ref<MaterialInstance> material, const glm::mat4& transform)
	{
		Renderable r = { mesh, material, transform };
		UUID materialID = material->ID();

		if (s_Renderables.find(materialID) == s_Renderables.end())
			s_Renderables[materialID] = std::vector<Renderable>();
		s_Renderables[materialID].push_back(r);
	}
	
	void Renderer::End()
	{
		Optimize();
		PrepareResources();
		DrawFrame();
	}

	void Renderer::Optimize()
	{

	}

	void Renderer::PrepareResources()
	{
		UpdateUniformBuffer(State::CurrentFramebufferIndex());

		VkFence waits[] = { *Synchronization::GetFence("FrameInFlight") };
		vkWaitForFences(VulkanCore::Device(), 1, waits, VK_TRUE, UINT64_MAX);

		// Acquire next image
		uint32_t imgIndex;
		VkResult res = vkAcquireNextImageKHR(VulkanCore::Device(), *s_Data.Swapchain, UINT64_MAX,
			*Synchronization::GetSemaphore("ImageAvailable"), VK_NULL_HANDLE, &imgIndex);
		if (res != VK_SUCCESS)
			throw std::runtime_error("Couldn't acquire image");
		State::SetCurrentImageIndex(imgIndex);

		// Reset everything
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}

		vkResetFences(VulkanCore::Device(), 1, waits);
		vkResetCommandBuffer(*s_Data.CommandBuffers[State::CurrentFramebufferIndex()], 0);
	}

	void Renderer::DrawFrame()
	{
		for (auto& batch : s_Renderables)
		{
			Ref<CommandBuffer> commandBuffer = s_Data.CommandBuffers[State::CurrentFramebufferIndex()];

			State::BindCommandBuffer(commandBuffer);
			commandBuffer->Begin();

			s_Data.RenderPass->Begin(s_Data.GraphicsPipeline, s_Data.Swapchain->Extent());
			{
				VkBuffer buffers[] = { *s_Data.Resources->Mesh->VertexBuffer() };
				VkDeviceSize offsets[] = { 0 };

				// Draw models
				{
					for (auto& model : batch.second)
					{
						vkCmdBindVertexBuffers(*commandBuffer, 0, 1, buffers, offsets);
						vkCmdBindIndexBuffer(*commandBuffer, *s_Data.Resources->Mesh->IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
						vkCmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.GraphicsPipeline->Layout(), 0, 1,
							&s_Data.DescriptorSets[State::CurrentFramebufferIndex()], 0, nullptr);

						PushConsts consts;

						consts.AO = 0.01f;
						consts.Metallic = 0.5f;
						consts.Roughness = 1.0f;
						consts.CameraPos = glm::vec3(0.0f, 2, 2);

						vkCmdPushConstants(*commandBuffer, s_Data.GraphicsPipeline->Layout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConsts), &consts);
						vkCmdDrawIndexed(*commandBuffer, s_Data.Resources->Mesh->IndexBuffer()->Size() / sizeof(uint32_t), 1, 0, 0, 0);
					}
				}
			}
			s_Data.RenderPass->End();
			commandBuffer->End();

			// Get submitted command buffers
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			std::vector<VkCommandBuffer> cmdBuffers(s_CommandBuffers.size());
			for (uint32_t i = 0; i < cmdBuffers.size(); i++)
				cmdBuffers[i] = *s_CommandBuffers[i];

			// Submit commands
			VkSubmitInfo submitInfo = {};
			VkSemaphore waits[] = { *Synchronization::GetSemaphore("ImageAvailable") };
			VkSemaphore signals[] = { *Synchronization::GetSemaphore("RenderFinished") };
			VkCommandBuffer buffers[] = { *s_Data.CommandBuffers[State::CurrentFramebufferIndex()] };

			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waits;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = buffers;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signals;

			if (vkQueueSubmit(*s_Data.GraphicsQueue, 1, &submitInfo, *Synchronization::GetFence("FrameInFlight")) != VK_SUCCESS)
				throw std::runtime_error("Couldn't submit queue for rendering");

			// Present
			uint32_t currImage = State::CurrentImageIndex();
			VkSwapchainKHR swapChains = { *s_Data.Swapchain };
			VkPresentInfoKHR presentInfo = {};
			waits[0] = *Synchronization::GetSemaphore("RenderFinished");

			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = waits;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapChains;
			presentInfo.pImageIndices = &currImage;
			presentInfo.pResults = nullptr;

			VkResult res = vkQueuePresentKHR(*s_Data.PresentationQueue, &presentInfo);
			if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
			{
				RecreateSwapchain();
				return;
			}
		}

		State::SetCurrentFrameIndex((State::CurrentFramebufferIndex() + 1) % s_Config.MaxFramesInFlight);
		State::SetFramebuffer(s_Data.Framebuffers[State::CurrentImageIndex()]);
		
	}

	void Renderer::Destroy()
	{
		Debug::Shutdown();
		
		vkDeviceWaitIdle(VulkanCore::Device());
		
		vkDestroyDescriptorPool(VulkanCore::Device(), s_Data.DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(VulkanCore::Device(), s_Data.DescriptorSetLayout, nullptr);

		delete s_Data.Resources;
	}
}