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

#include <Hardware/Support.h>
#include <Hardware/Memory.h>

#include <Structures/Buffer.h>
#include <Structures/Vertex.h>

#include <Resources/Texture.h>
#include <Resources/Shader.h>

#include <GLFW/glfw3.h>
#include <stb_image.h>

// This stuff should probably go in a VulkanCore class or something. Then expose globals init and shutdown that initialize and shutdown
// everything. Instances, devices and stuff like that don't have much to do with the actual renderer

namespace Low
{
	static  RendererConfig s_Config;

	struct RendererResources
	{
		// Buffers
		Ref<Buffer> VertexBuffer;
		Ref<Buffer> IndexBuffer;
		std::vector<Ref<Buffer>> UniformBuffers;

		// Resources
		Ref<Shader> Shader;
		Ref<Texture> Texture;
	};

	struct RendererData
	{
		// Device
		VkInstance Instance = VK_NULL_HANDLE;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VkDevice LogicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR WindowSurface = VK_NULL_HANDLE;
		VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		VkQueue PresentationQueue = VK_NULL_HANDLE;

		// Resources
		RendererResources* Resources;

		// Swapchain
		std::vector<VkImageView> SwapchainImageViews;
		std::vector<VkFramebuffer> SwapchainFramebuffers;

		VkFormat SwapchainImageFormat;
		VkExtent2D SwapchainExtent;

		// Depth buffer
		VkImage DepthImage;
		VkDeviceMemory DepthImageMemory;
		VkImageView DepthImageView;

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
			vkGetPhysicalDeviceFormatProperties(s_Data.PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		throw std::runtime_error("Couldn't find a supported depth format");
	}

	static bool HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	static VkFormat FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	static VkCommandBuffer BeginOneTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		VkCommandBuffer commandBuffer;

		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = s_Data.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(s_Data.LogicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Couldn't allocate command buffers");
		}
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("Couldn't begin cmd buffer");

		return commandBuffer;
	}

	static void EndOneTimeCommands(VkCommandBuffer cmdBuffer)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;

		vkQueueSubmit(s_Data.GraphicsQueue, 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(s_Data.GraphicsQueue);

		vkFreeCommandBuffers(s_Data.LogicalDevice, s_Data.CommandPool, 1, &cmdBuffer);
	}

	static void CreateFramebuffer()
	{
		s_Data.SwapchainFramebuffers.resize(s_Data.SwapchainImageViews.size());
		for (uint32_t i = 0; i < s_Data.SwapchainFramebuffers.size(); i++)
		{
			std::array<VkImageView, 2> attachments = { s_Data.SwapchainImageViews[i], s_Data.DepthImageView };
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = s_Data.RenderPass;
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = s_Data.SwapchainExtent.width;
			framebufferInfo.height = s_Data.SwapchainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(s_Data.LogicalDevice, &framebufferInfo, nullptr, &s_Data.SwapchainFramebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Couldn't create framebuffer");
		}
	}

	static void TransitionImageToLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer cmdBuf = BeginOneTimeCommands();
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;

			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags destinationStage;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_NONE;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (HasStencilComponent(format))
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			vkCmdPipelineBarrier(cmdBuf, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		}
		EndOneTimeCommands(cmdBuf);
	}

	static void CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();

		VkDeviceSize size = s_Data.SwapchainExtent.width * s_Data.SwapchainExtent.height * 4;

		VkImageCreateInfo texInfo = {};
		texInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texInfo.imageType = VK_IMAGE_TYPE_2D;
		texInfo.extent.width = s_Data.SwapchainExtent.width;
		texInfo.extent.height = s_Data.SwapchainExtent.height;
		texInfo.extent.depth = 1;
		texInfo.mipLevels = 1;
		texInfo.arrayLayers = 1;
		texInfo.format = depthFormat;
		texInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		texInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		texInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		texInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		texInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		texInfo.flags = 0;

		if (vkCreateImage(s_Data.LogicalDevice, &texInfo, nullptr, &s_Data.DepthImage) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create texture image");

		VkMemoryRequirements memoryReqs;
		vkGetImageMemoryRequirements(s_Data.LogicalDevice, s_Data.DepthImage, &memoryReqs);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryReqs.size;
		allocInfo.memoryTypeIndex = Memory::FindMemoryType(s_Data.PhysicalDevice, memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(s_Data.LogicalDevice, &allocInfo, nullptr, &s_Data.DepthImageMemory) != VK_SUCCESS)
			throw std::runtime_error("Couldn't allocate texture memory");

		vkBindImageMemory(s_Data.LogicalDevice, s_Data.DepthImage, s_Data.DepthImageMemory, 0);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = s_Data.DepthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(s_Data.LogicalDevice, &viewInfo, nullptr, &s_Data.DepthImageView) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create image view");

		TransitionImageToLayout(s_Data.DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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
		renderPassInfo.framebuffer = s_Data.SwapchainFramebuffers[imageIndex];
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

			VkBuffer buffers[] = { s_Data.Resources->VertexBuffer->Handle()};
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.GraphicsPipeline);

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissors);

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, s_Data.Resources->IndexBuffer->Handle(), 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.PipelineLayout, 0, 1,
				&s_Data.DescriptorSets[s_State.CurrentFrame], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, s_Data.Resources->IndexBuffer->Size() / sizeof(uint16_t), 1, 0, 0, 0);
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

			if (vkCreateSemaphore(s_Data.LogicalDevice, &semInfo, nullptr, &s_Synch[i].SemImageAvailable) != VK_SUCCESS ||
				vkCreateSemaphore(s_Data.LogicalDevice, &semInfo, nullptr, &s_Synch[i].SemRenderFinished) != VK_SUCCESS ||
				vkCreateFence(s_Data.LogicalDevice, &fenInfo, nullptr, &s_Synch[i].FenInFlight) != VK_SUCCESS)
				throw std::runtime_error("Couldn't create synchronization structures");
		}
	}

	static void CleanupSwapchain()
	{
		for (auto& image : s_Data.SwapchainImageViews)
			vkDestroyImageView(s_Data.LogicalDevice, image, nullptr);
		for (auto& buf : s_Data.SwapchainFramebuffers)
			vkDestroyFramebuffer(s_Data.LogicalDevice, buf, nullptr);

		vkDestroyImage(s_Data.LogicalDevice, s_Data.DepthImage, nullptr);
		vkDestroyImageView(s_Data.LogicalDevice, s_Data.DepthImageView, nullptr);
		vkFreeMemory(s_Data.LogicalDevice, s_Data.DepthImageMemory, nullptr);

		vkDestroySwapchainKHR(s_Data.LogicalDevice, s_Data.Swapchain, nullptr);
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

		vkDeviceWaitIdle(s_Data.LogicalDevice);

		CleanupSwapchain();

		//CreateSwapchain();
		//CreateImageViews();
		CreateDepthResources();
		CreateFramebuffer();
	}

	static void OnFramebufferResize(GLFWwindow* window, int width, int height)
	{
		s_State.FramebufferResized = true;
	}

	static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(s_Data.PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props)
				return i;
		}

		throw std::runtime_error("Couldn't find suitable memory type");
	}

	static void CopyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginOneTimeCommands();
		{
			VkBufferCopy copyRegion;
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
		}
		EndOneTimeCommands(commandBuffer);
	}

	static void CreateVertexBuffer()
	{
		const std::vector<Vertex> vertices = {
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
			{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

			{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
			{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
		};

		VkDeviceSize size = sizeof(Vertex) * vertices.size();

		s_Data.Resources->VertexBuffer = CreateRef<Buffer>(s_Data.LogicalDevice, s_Data.PhysicalDevice, size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		Buffer stagingBuffer = Buffer(s_Data.LogicalDevice, s_Data.PhysicalDevice, size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		void* data;
		if (vkMapMemory(s_Data.LogicalDevice, stagingBuffer.Memory(), 0, size, 0, &data) != VK_SUCCESS)
			std::cout << "Mapping failed" << std::endl;
		memcpy(data, vertices.data(), (size_t)size);
		vkUnmapMemory(s_Data.LogicalDevice, stagingBuffer.Memory());

		CopyBuffer(s_Data.Resources->VertexBuffer->Handle(), stagingBuffer.Handle(), size);
	}

	static void CreateIndexBuffer()
	{
		const std::vector<uint16_t> indices = { 
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4
		};
		VkDeviceSize size = sizeof(uint16_t) * indices.size();

		s_Data.Resources->IndexBuffer = CreateRef<Buffer>(s_Data.LogicalDevice, s_Data.PhysicalDevice, size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		Buffer stagingBuffer = Buffer(s_Data.LogicalDevice, s_Data.PhysicalDevice, size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(s_Data.LogicalDevice, stagingBuffer.Memory(), 0, size, 0, &data);
		memcpy(data, indices.data(), (size_t)size);
		vkUnmapMemory(s_Data.LogicalDevice, stagingBuffer.Memory());

		CopyBuffer(s_Data.Resources->IndexBuffer->Handle(), stagingBuffer.Handle(), size);
	}

	static void CreateUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		s_Data.Resources->UniformBuffers.resize(s_Config.MaxFramesInFlight);
		s_Data.UniformBuffersMemory.resize(s_Config.MaxFramesInFlight);
		s_Data.UniformBuffersMapped.resize(s_Config.MaxFramesInFlight);

		for (uint32_t i = 0; i < s_Config.MaxFramesInFlight; i++)
		{
			s_Data.Resources->UniformBuffers[i] = CreateRef<Buffer>(s_Data.LogicalDevice, s_Data.PhysicalDevice, bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			vkMapMemory(s_Data.LogicalDevice, s_Data.Resources->UniformBuffers[i]->Memory(), 0, bufferSize, 0, &s_Data.UniformBuffersMapped[i]);
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
		if (vkAllocateDescriptorSets(s_Data.LogicalDevice, &allocateInfo, s_Data.DescriptorSets.data()) != VK_SUCCESS)
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

			vkUpdateDescriptorSets(s_Data.LogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
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

	static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer cmdBuf = BeginOneTimeCommands();
		{
			VkBufferImageCopy reg = {};

			reg.bufferOffset = 0;
			reg.bufferRowLength = 0;
			reg.bufferImageHeight = 0;

			reg.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			reg.imageSubresource.mipLevel = 0;
			reg.imageSubresource.layerCount = 1;
			reg.imageSubresource.baseArrayLayer = 0;

			reg.imageOffset = { 0,0,0 };
			reg.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(cmdBuf, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
		}
		EndOneTimeCommands(cmdBuf);
	}
	

	static void CreateTextureImage()
	{
		s_Data.Resources->Texture = CreateRef<Texture>("../textures/texture.jpg", s_Data.LogicalDevice, s_Data.PhysicalDevice, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL);
		auto tex = s_Data.Resources->Texture;

		TransitionImageToLayout(tex->Handle(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(tex->Buffer(), tex->Handle(), tex->GetWidth(), tex->GetHeight());
		TransitionImageToLayout(tex->Handle(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
		vkGetPhysicalDeviceProperties(s_Data.PhysicalDevice, &properties);

		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(s_Data.LogicalDevice, &samplerInfo, nullptr, s_Data.Resources->Texture->Sampler()) != VK_SUCCESS)
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
		
		s_Data.Instance = VulkanCore::Instance();
		s_Data.PhysicalDevice = VulkanCore::PhysicalDevice();
		s_Data.LogicalDevice = VulkanCore::Device();
		s_Data.WindowSurface = VulkanCore::Surface();
		s_Data.GraphicsQueue = VulkanCore::GraphicsQueue();
		s_Data.PresentationQueue = VulkanCore::PresentQueue();

		Ref<Swapchain> swapchain = CreateRef<Swapchain>(s_Data.WindowSurface, width, height);

		s_Data.Swapchain = swapchain->Handle();
		s_Data.SwapchainExtent = { (uint32_t)width, (uint32_t)height };
		s_Data.SwapchainImageFormat = swapchain->Format();
		s_Data.SwapchainImageViews.resize(swapchain->ImageViews().size());
		for (uint32_t i = 0; i < swapchain->ImageViews().size(); i++)
			s_Data.SwapchainImageViews[i] = swapchain->ImageViews()[i];

		Ref<RenderPass> renderPass = CreateRef<RenderPass>(swapchain->Format(), FindDepthFormat());
		s_Data.RenderPass = renderPass->Handle();

		glfwSetFramebufferSizeCallback(windowHandle, OnFramebufferResize);

		Ref<DescriptorSetLayout> descriptorSetLayout = CreateRef<DescriptorSetLayout>();
		s_Data.DescriptorSetLayout = descriptorSetLayout->Handle();

		Ref<DescriptorPool> descriptorPool = CreateRef<DescriptorPool>(s_Config.MaxFramesInFlight);
		s_Data.DescriptorPool = descriptorPool->Handle();

		CreateUniformBuffers();

		Ref<Shader> shader = CreateRef<Shader>("basic", s_Data.LogicalDevice);
		s_Data.Resources->Shader = shader;
		Ref<GraphicsPipeline> graphicsPipeline = CreateRef<GraphicsPipeline>(shader, descriptorSetLayout, renderPass, glm::vec2(s_Data.SwapchainExtent.width, s_Data.SwapchainExtent.height));
		s_Data.GraphicsPipeline = graphicsPipeline->Handle();
		s_Data.PipelineLayout = graphicsPipeline->Layout();

		Ref<CommandPool> commandPool = CreateRef<CommandPool>(Support::GetQueueFamilyIndices(s_Data.PhysicalDevice, s_Data.WindowSurface));
		s_Data.CommandPool = commandPool->Handle();

		std::vector<Ref<CommandBuffer>> commandBuffers = commandPool->AllocateCommandBuffers(s_Config.MaxFramesInFlight);
		for (auto& buf : commandBuffers)
			s_Data.CommandBuffers.push_back(buf->Handle());

		CreateDepthResources();
		CreateFramebuffer();
		
		CreateTextureImage();
		CreateTextureSampler();

		CreateVertexBuffer();
		CreateIndexBuffer();

		CreateDescriptorSets();

		CreateSynchronizationObjects();
	}
	

	void Renderer::DrawFrame()
	{
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		uint32_t imageIdx;

		UpdateUniformBuffer(s_State.CurrentFrame);
		vkWaitForFences(s_Data.LogicalDevice, 1, &s_Synch[s_State.CurrentFrame].FenInFlight, VK_TRUE, UINT64_MAX);

		VkResult res = vkAcquireNextImageKHR(s_Data.LogicalDevice, s_Data.Swapchain, UINT64_MAX, 
			s_Synch[s_State.CurrentFrame].SemImageAvailable, VK_NULL_HANDLE, &imageIdx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}

		vkResetFences(s_Data.LogicalDevice, 1, &s_Synch[s_State.CurrentFrame].FenInFlight);
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
		
		vkDeviceWaitIdle(s_Data.LogicalDevice);

		CleanupSwapchain();

		vkDestroyPipelineLayout(s_Data.LogicalDevice, s_Data.PipelineLayout, nullptr);
		vkDestroyPipeline(s_Data.LogicalDevice, s_Data.GraphicsPipeline, nullptr);
		vkDestroyRenderPass(s_Data.LogicalDevice, s_Data.RenderPass, nullptr);

		vkDestroyCommandPool(s_Data.LogicalDevice, s_Data.CommandPool, nullptr);

		for (uint32_t i = 0; i < s_Synch.size(); i++)
		{
			vkDestroySemaphore(s_Data.LogicalDevice, s_Synch[i].SemImageAvailable, nullptr);
			vkDestroySemaphore(s_Data.LogicalDevice, s_Synch[i].SemRenderFinished, nullptr);
			vkDestroyFence(s_Data.LogicalDevice, s_Synch[i].FenInFlight, nullptr);
		}

		vkDestroyDescriptorPool(s_Data.LogicalDevice, s_Data.DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(s_Data.LogicalDevice, s_Data.DescriptorSetLayout, nullptr);

		delete s_Data.Resources;
		vkDestroyImage(s_Data.LogicalDevice, s_Data.DepthImage, nullptr);
		vkDestroyImageView(s_Data.LogicalDevice, s_Data.DepthImageView, nullptr);
		vkFreeMemory(s_Data.LogicalDevice, s_Data.DepthImageMemory, nullptr);
		vkDestroyDevice(s_Data.LogicalDevice, nullptr);

		vkDestroySurfaceKHR(s_Data.Instance, s_Data.WindowSurface, nullptr);
		vkDestroyInstance(s_Data.Instance, nullptr);

	}
}