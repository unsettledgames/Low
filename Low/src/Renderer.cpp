#include <iostream>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <algorithm>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Core/Core.h>
#include <Core/Debug.h>
#include <Renderer.h>

#include <Structures/Buffer.h>
#include <Resources/Texture.h>
#include <Resources/Shader.h>

#include <vulkan/vulkan.h>
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
		std::vector<VkImage> SwapchainImages;
		std::vector<VkImageView> SwapchainImageViews;
		VkFormat SwapchainImageFormat;
		VkExtent2D SwapchainExtent;

		// Pipeline
		VkPipelineLayout PipelineLayout;
		VkRenderPass RenderPass;
		VkPipeline GraphicsPipeline;
		std::vector<VkFramebuffer> SwapchainFramebuffers;

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

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

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
			std::vector<VkVertexInputAttributeDescription> ret(2);

			ret[0].binding = 0;
			ret[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			ret[0].offset = 0;
			ret[0].location = 0;

			ret[1].binding = 0;
			ret[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			ret[1].offset = sizeof(glm::vec3);
			ret[1].location = 1;

			return ret;
		}
	};


	std::vector<Synch> s_Synch;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> Graphics;
		std::optional<uint32_t> Presentation;

		bool Complete() { return Graphics.has_value() && Presentation.has_value(); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	static SwapchainSupportDetails GetSwapchainSupportDetails(VkPhysicalDevice device)
	{
		SwapchainSupportDetails ret;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, s_Data.WindowSurface, &ret.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_Data.WindowSurface, &formatCount, nullptr);
		ret.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, s_Data.WindowSurface, &formatCount, ret.Formats.data());

		uint32_t presentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_Data.WindowSurface, &presentModesCount, nullptr);
		ret.PresentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, s_Data.WindowSurface, &presentModesCount, ret.PresentModes.data());

		return ret;
	}

	static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device)
	{
		QueueFamilyIndices ret;

		uint32_t nQueues;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueues, nullptr);
		std::vector<VkQueueFamilyProperties> queueProps(nQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueues, queueProps.data());

		uint32_t i = 0;

		for (auto family : queueProps)
		{
			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				ret.Graphics = i;

			VkBool32 present;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, s_Data.WindowSurface, &present);
			if (present)
				ret.Presentation = i;

			if (ret.Graphics.has_value() && ret.Presentation.has_value())
				break;
			i++;
		}

		return ret;
	}

	static void CreateVkInstance(const char** extensions, uint32_t nExtensions)
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> vkextensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkextensions.data());
		std::cout << "available extensions:\n";

		for (const auto& extension : vkextensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "LowRenderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo = {};
		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
		Debug::GetCreateInfo(debugInfo);

		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.pNext = &debugInfo;

		createInfo.enabledExtensionCount = nExtensions;
		createInfo.ppEnabledExtensionNames = extensions;

#ifndef LOW_VALIDATION_LAYERS
		createInfo.enabledLayerCount = 0;
#else
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();

#endif

		VkResult ret = vkCreateInstance(&createInfo, nullptr, &s_Data.Instance);
		if (ret != VK_SUCCESS)
			std::cout << "Instance creation failure: " << ret << std::endl;
	}

	static float GetPhysicalDeviceScore(VkPhysicalDevice device)
	{
		float ret = 0;
		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memoryProps;

		vkGetPhysicalDeviceProperties(device, &props);
		vkGetPhysicalDeviceFeatures(device, &features);
		vkGetPhysicalDeviceMemoryProperties(device, &memoryProps);

		// GPU properties
		if (props.deviceType = VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			ret += 1000.0f;
		if (features.samplerAnisotropy)
			ret += 1.0f;

		for (uint32_t i = 0; i < memoryProps.memoryHeapCount; i++)
			ret += memoryProps.memoryHeaps[i].size / 1000000;
		
		// Queues
		auto queueIndices = GetQueueFamilyIndices(device);
		if (!queueIndices.Complete())
			return 0;
		if (queueIndices.Graphics.value() != queueIndices.Presentation.value())
			ret -= 100.0f;

		// Extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<const char*> requiredExtensions(s_Data.RequiredExtesions.begin(), s_Data.RequiredExtesions.end());

		for (auto ext : availableExtensions)
			requiredExtensions.erase(ext.extensionName);
		if (!requiredExtensions.empty())
			return 0;

		return ret;
	}

	static void PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(s_Data.Instance, &deviceCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(s_Data.Instance, &deviceCount, physicalDevices.data());

		VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
		float currScore = -1;

		for (auto device : physicalDevices)
		{
			float score = GetPhysicalDeviceScore(device);
			if (score > currScore)
			{
				currScore = score;
				bestDevice = device;
			}
		}

		if (bestDevice == VK_NULL_HANDLE)
			std::cerr << "Couldn't find a suitable GPU" << std::endl;
		s_Data.PhysicalDevice = bestDevice;
	}

	static void CreateLogicalDevice()
	{
		QueueFamilyIndices indices = GetQueueFamilyIndices(s_Data.PhysicalDevice);
		float priority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfo;
		std::set<uint32_t> queueFamilies = { indices.Graphics.value(), indices.Presentation.value()};

		VkDeviceCreateInfo createInfo = {};

		for (uint32_t family : queueFamilies)
		{
			VkDeviceQueueCreateInfo queueInfo = {};

			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = family;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &priority;

			queueCreateInfo.push_back(queueInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfo.data();
		createInfo.queueCreateInfoCount = queueCreateInfo.size();
		createInfo.pEnabledFeatures = &deviceFeatures;
		
		createInfo.enabledExtensionCount = 1;
		createInfo.ppEnabledExtensionNames = s_Data.RequiredExtesions.data();

#ifdef LOW_VALIDATION_LAYERS
		auto layers = Debug::GetValidationLayers();
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif

		auto err = vkCreateDevice(s_Data.PhysicalDevice, &createInfo, nullptr, &s_Data.LogicalDevice);
		if (err != VK_SUCCESS)
			std::cerr << "Failed creating logical device" << std::endl;

		vkGetDeviceQueue(s_Data.LogicalDevice, indices.Graphics.value(), 0, &s_Data.GraphicsQueue);
		vkGetDeviceQueue(s_Data.LogicalDevice, indices.Presentation.value(), 0, &s_Data.PresentationQueue);
	}

	static void CreateWindowSurface()
	{
		if (glfwCreateWindowSurface(s_Data.Instance, s_Data.WindowHandle, nullptr, &s_Data.WindowSurface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface!");
	}

	static void CreateSwapchain()
	{
		// Swapchain properties
		SwapchainSupportDetails swapchainProps = GetSwapchainSupportDetails(s_Data.PhysicalDevice);
		if (swapchainProps.Formats.empty() || swapchainProps.PresentModes.empty())
			throw std::runtime_error("Swapchain has no formats or present modes");

		// Color format
		VkSurfaceFormatKHR swapchainFormat;
		bool foundFormat = false;

		for (auto format : swapchainProps.Formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				swapchainFormat = format;
				foundFormat = true;
				break;
			}
		}
		// For lack of better formats, use the first one
		if (!foundFormat)
			swapchainFormat = swapchainProps.Formats[0];

		// Presentation mode
		VkPresentModeKHR presentMode;
		bool foundPresentation = false;

		for (auto presentation : swapchainProps.PresentModes)
		{
			if (presentation == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = presentation;
				foundPresentation = true;
				break;
			}
		}
		// Same concept as the format
		if (!foundPresentation)
			presentMode = VK_PRESENT_MODE_FIFO_KHR;

		VkExtent2D extent;
		// Swap extent
		if (swapchainProps.Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			extent = swapchainProps.Capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(s_Data.WindowHandle, &width, &height);
			extent = { (uint32_t)width, (uint32_t)height };
			extent.width = std::clamp(extent.width, swapchainProps.Capabilities.minImageExtent.width, swapchainProps.Capabilities.maxImageExtent.width);
			extent.height = std::clamp(extent.height, swapchainProps.Capabilities.minImageExtent.height, swapchainProps.Capabilities.maxImageExtent.height);
		}

		uint32_t imageCount = swapchainProps.Capabilities.minImageCount;
		if (imageCount != swapchainProps.Capabilities.maxImageCount)
			imageCount++;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = s_Data.WindowSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = swapchainFormat.format;
		createInfo.imageColorSpace = swapchainFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto indice = GetQueueFamilyIndices(s_Data.PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indice.Graphics.value(), indice.Presentation.value() };

		if (indice.Graphics != indice.Presentation) 
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else 
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapchainProps.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult res = vkCreateSwapchainKHR(s_Data.LogicalDevice, &createInfo, nullptr, &s_Data.Swapchain);
		if (res != VK_SUCCESS)
			std::cerr << "Couldn't create swapchain" << std::endl;

		imageCount;
		vkGetSwapchainImagesKHR(s_Data.LogicalDevice, s_Data.Swapchain, &imageCount, nullptr);
		s_Data.SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(s_Data.LogicalDevice, s_Data.Swapchain, &imageCount, s_Data.SwapchainImages.data());

		s_Data.SwapchainExtent = extent;
		s_Data.SwapchainImageFormat = swapchainFormat.format;
	}

	static VkCommandBuffer BeginOneTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		VkCommandBuffer commandBuffer;
		s_Data.CommandBuffers.resize(s_Config.MaxFramesInFlight);

		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = s_Data.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = s_Data.CommandBuffers.size();

		if (vkAllocateCommandBuffers(s_Data.LogicalDevice, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Couldn't allocate command buffers");
		}
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

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

	void CreateImageViews()
	{
		s_Data.SwapchainImageViews.resize(s_Data.SwapchainImages.size());
		for (uint32_t i = 0; i < s_Data.SwapchainImageViews.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = s_Data.SwapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = s_Data.SwapchainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(s_Data.LogicalDevice, &createInfo, nullptr, &s_Data.SwapchainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error("Couldn't create image view");
		}
	}

	static void CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = s_Data.SwapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		// The index is the index in the glsl shader layout!
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dep = {};
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass = 0;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.srcAccessMask = 0;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dep;

		if (vkCreateRenderPass(s_Data.LogicalDevice, &renderPassInfo, nullptr, &s_Data.RenderPass))
			throw std::runtime_error("Failed to create render pass");
	}

	static void CreateGraphicsPipeline()
	{
		s_Data.Resources->Shader = CreateRef<Shader>("basic", s_Data.LogicalDevice);
		
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = s_Data.Resources->Shader->GetVertexModule();
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = s_Data.Resources->Shader->GetFragmentModule();
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		auto bindingDesc = Vertex::GetVertexBindingDescription();
		auto attributeDesc = Vertex::GetVertexAttributeDescriptions();

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount =	1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDesc.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)s_Data.SwapchainExtent.width;
		viewport.height = (float)s_Data.SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = s_Data.SwapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pScissors = &scissor;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.viewportCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		// [SHADOWMAPPING]
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// [ANTIALIASING]
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; 
		colorBlending.blendConstants[1] = 0.0f; 
		colorBlending.blendConstants[2] = 0.0f; 
		colorBlending.blendConstants[3] = 0.0f; 

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &s_Data.DescriptorSetLayout;

		if (vkCreatePipelineLayout(s_Data.LogicalDevice, &pipelineLayoutInfo, nullptr, &s_Data.PipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create pipeline layout!");

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;

		pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &rasterizer;
		pipelineCreateInfo.pMultisampleState = &multisampling;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pColorBlendState = &colorBlending;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.layout = s_Data.PipelineLayout;
		pipelineCreateInfo.renderPass = s_Data.RenderPass;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(s_Data.LogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &s_Data.GraphicsPipeline) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create graphics pipeline");
	}

	static void CreateFramebuffer()
	{
		s_Data.SwapchainFramebuffers.resize(s_Data.SwapchainImageViews.size());
		for (uint32_t i = 0; i < s_Data.SwapchainFramebuffers.size(); i++)
		{
			VkImageView attachments[] = { s_Data.SwapchainImageViews[i] };
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = s_Data.RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = s_Data.SwapchainExtent.width;
			framebufferInfo.height = s_Data.SwapchainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(s_Data.LogicalDevice, &framebufferInfo, nullptr, &s_Data.SwapchainFramebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Couldn't create framebuffer");
		}
	}

	static void CreateCommandPool()
	{
		auto queueIndices = GetQueueFamilyIndices(s_Data.PhysicalDevice);
		VkCommandPoolCreateInfo commandPoolInfo = {};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = queueIndices.Graphics.value();

		if (vkCreateCommandPool(s_Data.LogicalDevice, &commandPoolInfo, nullptr, &s_Data.CommandPool) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create command pool");
	}

	static void CreateCommandBuffers()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		s_Data.CommandBuffers.resize(s_Config.MaxFramesInFlight);
		
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = s_Data.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = s_Data.CommandBuffers.size();

		if (vkAllocateCommandBuffers(s_Data.LogicalDevice, &allocInfo, s_Data.CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Couldn't allocate command buffers");
		}
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

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

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

			vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
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

		CreateSwapchain();
		CreateImageViews();
		CreateFramebuffer();
	}

	static void OnFramebufferResize(GLFWwindow* window, int width, int height)
	{
		s_State.FramebufferResized = true;
		std::cout << "Resized!" << std::endl;
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
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
			{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
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
		const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
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

	static void CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize,2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = s_Config.MaxFramesInFlight;
		
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = s_Config.MaxFramesInFlight;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = s_Config.MaxFramesInFlight;

		if (vkCreateDescriptorPool(s_Data.LogicalDevice, &poolInfo, nullptr, &s_Data.DescriptorPool) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create descriptor pool");
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

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = s_Data.DescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(s_Data.LogicalDevice, 1, &descriptorWrite, 0, nullptr);
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
	
	void CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboBinding = {};
		uboBinding.binding = 0;
		uboBinding.descriptorCount = 1;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerBinding = {};
		samplerBinding.binding = 1;
		samplerBinding.descriptorCount = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		VkPipelineLayout pipelineLayout = {};

		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(s_Data.LogicalDevice, &layoutInfo, nullptr, &s_Data.DescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create descriptor set layout");
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

			vkCmdPipelineBarrier(cmdBuf, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

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
		s_Config = config;
		s_Data.Resources = new RendererResources();

		s_Data.WindowHandle = windowHandle;
		glfwSetFramebufferSizeCallback(s_Data.WindowHandle, OnFramebufferResize);

		Debug::InitValidationLayers();
		CreateVkInstance(config.Extensions, config.ExtensionCount);
		Debug::InitMessengers(s_Data.Instance);

		CreateWindowSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();

		CreateSwapchain();
		CreateImageViews();

		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFramebuffer();

		CreateCommandPool();
		CreateCommandBuffers();
		
		CreateTextureImage();
		CreateTextureSampler();

		CreateVertexBuffer();
		CreateIndexBuffer();

		CreateUniformBuffers();
		CreateDescriptorPool();
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
		vkDestroyDevice(s_Data.LogicalDevice, nullptr);

		vkDestroySurfaceKHR(s_Data.Instance, s_Data.WindowSurface, nullptr);
		vkDestroyInstance(s_Data.Instance, nullptr);

	}
}