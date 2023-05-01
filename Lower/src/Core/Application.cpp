#include <Vulkan/Command/ImmediateCommands.h>
#include <Core/Application.h>

#include <Hardware/Support.h>
#include <Vulkan/VulkanCore.h>
#include <Vulkan/RenderPass.h>
#include <Vulkan/Queue.h>
#include <Renderer.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

namespace Lower
{
    Application* Application::s_Application;

    Application::Application(const std::string& appName, uint32_t windowWidth, uint32_t windowHeight) :
        m_Name(appName), m_Width(windowWidth), m_Height(windowHeight) 
    {
        s_Application = this;
    }

    void Application::InitImGui()
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool imguiPool;
        vkCreateDescriptorPool(Low::VulkanCore::Device(), &pool_info, nullptr, &imguiPool);

        VkAttachmentDescription attachment = {};
        attachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;

        VkRenderPass imGuiRenderPass = VK_NULL_HANDLE;
        if (vkCreateRenderPass(Low::VulkanCore::Device(), &info, nullptr, &imGuiRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("Could not create Dear ImGui's render pass");
        }


        // 2: initialize imgui library
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(Application::Get()->WindowHandle(), true);

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = Low::VulkanCore::Instance();
        init_info.PhysicalDevice = Low::VulkanCore::PhysicalDevice();
        init_info.Device = Low::VulkanCore::Device();
        init_info.Queue = *Low::VulkanCore::GraphicsQueue();
        init_info.DescriptorPool = imguiPool;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info, imGuiRenderPass);

        //execute a gpu command to upload imgui font textures
        VkCommandBuffer buf = Low::ImmediateCommands::Begin();
        ImGui_ImplVulkan_CreateFontsTexture(buf);
        Low::ImmediateCommands::End(buf);

        //clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void Application::InitRenderer()
    {
        std::vector<const char*> extensions;
        uint32_t extensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        for (uint32_t i = 0; i < extensionCount; i++)
            extensions.push_back(glfwExtensions[i]);

        extensions.push_back("VK_EXT_debug_utils");
        extensionCount++;

        Low::RendererConfig config;
        config.ExtensionCount = extensionCount;
        config.Extensions = extensions.data();
        config.MaxFramesInFlight = 2;

        Low::Renderer::Init(config, m_WindowHandle);
    }

	void Application::Init()
	{
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_WindowHandle = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);

        InitRenderer();
        InitImGui();
	}

	void Application::Run()
	{
        while (!glfwWindowShouldClose(m_WindowHandle)) 
        {
            glfwPollEvents();
            Low::Renderer::Begin();
            Low::Renderer::DrawFrame();
            Low::Renderer::End();
        }

        Stop();
	}

    void Application::Stop()
    {
        glfwDestroyWindow(m_WindowHandle);
        glfwTerminate();

        Low::Renderer::Destroy();
    }
}