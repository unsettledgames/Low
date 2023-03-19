#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>

#include <Core/Core.h>
#include <Core/Debug.h>
#include <Renderer.h>
#include <Resources/Shader.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


// This stuff should probably go in a VulkanCore class or something. Then expose globals init and shutdown that initialize and shutdown
// everything. Instances, devices and stuff like that don't have much to do with the actual renderer

namespace Low
{
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
		VkCommandBuffer CommandBuffer;

		GLFWwindow* WindowHandle;
		std::vector<const char*> RequiredExtesions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	} s_Data;

	struct Synch
	{
		VkSemaphore SemImageAvailable;
		VkSemaphore SemRenderFinished;
		VkFence FenInFlight;
	} s_Synch;

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
		Ref<Shader> shader = CreateRef<Shader>("basic", s_Data.LogicalDevice);
		
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shader->GetVertexModule();
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shader->GetFragmentModule();
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

	static void CreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = s_Data.CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(s_Data.LogicalDevice, &allocInfo, &s_Data.CommandBuffer) != VK_SUCCESS) {
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

		vkCmdBeginRenderPass(s_Data.CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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

			vkCmdBindPipeline(s_Data.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Data.GraphicsPipeline);

			vkCmdSetViewport(s_Data.CommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(s_Data.CommandBuffer, 0, 1, &scissors);

			vkCmdDraw(s_Data.CommandBuffer, 3, 1, 0, 0);
		}
		vkCmdEndRenderPass(s_Data.CommandBuffer);

		if (vkEndCommandBuffer(s_Data.CommandBuffer) != VK_SUCCESS)
			throw std::runtime_error("Error ending the command buffer");
	}

	static void CreateSynchronizationObjects()
	{
		VkSemaphoreCreateInfo semInfo = {};
		VkFenceCreateInfo fenInfo = {};

		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		fenInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(s_Data.LogicalDevice, &semInfo, nullptr, &s_Synch.SemImageAvailable) != VK_SUCCESS ||
			vkCreateSemaphore(s_Data.LogicalDevice, &semInfo, nullptr, &s_Synch.SemRenderFinished) != VK_SUCCESS ||
			vkCreateFence(s_Data.LogicalDevice, &fenInfo, nullptr, &s_Synch.FenInFlight) != VK_SUCCESS)
			throw std::runtime_error("Couldn't create synchronization structures");
	}

	void Renderer::Init(const char** extensions, uint32_t nExtensions, GLFWwindow* handle)
	{
		s_Data.WindowHandle = handle;

		Debug::InitValidationLayers();
		CreateVkInstance(extensions, nExtensions);
		Debug::InitMessengers(s_Data.Instance);

		CreateWindowSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();

		CreateSwapchain();
		CreateImageViews();

		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffer();

		CreateCommandPool();
		CreateCommandBuffer();

		CreateSynchronizationObjects();
	}

	void Renderer::DrawFrame()
	{
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		uint32_t imageIdx;

		vkWaitForFences(s_Data.LogicalDevice, 1, &s_Synch.FenInFlight, VK_TRUE, UINT64_MAX);
		vkResetFences(s_Data.LogicalDevice, 1, &s_Synch.FenInFlight);

		vkAcquireNextImageKHR(s_Data.LogicalDevice, s_Data.Swapchain, UINT64_MAX, s_Synch.SemImageAvailable, VK_NULL_HANDLE, &imageIdx);

		vkResetCommandBuffer(s_Data.CommandBuffer, 0);
		RecordCommandBuffer(s_Data.CommandBuffer, imageIdx);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &s_Synch.SemImageAvailable;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &s_Data.CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &s_Synch.SemRenderFinished;

		if (vkQueueSubmit(s_Data.GraphicsQueue, 1, &submitInfo, s_Synch.FenInFlight) != VK_SUCCESS)
			throw std::runtime_error("Couldn't submit queue for rendering");

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &s_Synch.SemRenderFinished;
		 
		VkSwapchainKHR swapChains = { s_Data.Swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChains;
		presentInfo.pImageIndices = &imageIdx;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(s_Data.PresentationQueue, &presentInfo);
	}

	void Renderer::Destroy()
	{
		Debug::Shutdown();

		for (auto& image : s_Data.SwapchainImageViews)
			vkDestroyImageView(s_Data.LogicalDevice, image, nullptr);
		for (auto& buf : s_Data.SwapchainFramebuffers)
			vkDestroyFramebuffer(s_Data.LogicalDevice, buf, nullptr);

		vkDestroySwapchainKHR(s_Data.LogicalDevice, s_Data.Swapchain, nullptr);
		vkDestroyPipelineLayout(s_Data.LogicalDevice, s_Data.PipelineLayout, nullptr);
		vkDestroyPipeline(s_Data.LogicalDevice, s_Data.GraphicsPipeline, nullptr);
		vkDestroyRenderPass(s_Data.LogicalDevice, s_Data.RenderPass, nullptr);

		vkDestroyCommandPool(s_Data.LogicalDevice, s_Data.CommandPool, nullptr);

		vkDestroySemaphore(s_Data.LogicalDevice, s_Synch.SemImageAvailable, nullptr);
		vkDestroySemaphore(s_Data.LogicalDevice, s_Synch.SemRenderFinished, nullptr);
		vkDestroyFence(s_Data.LogicalDevice, s_Synch.FenInFlight, nullptr);

		vkDestroyDevice(s_Data.LogicalDevice, nullptr);

		vkDestroySurfaceKHR(s_Data.Instance, s_Data.WindowSurface, nullptr);
		vkDestroyInstance(s_Data.Instance, nullptr);

	}
}