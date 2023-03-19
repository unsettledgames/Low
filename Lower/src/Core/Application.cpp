#include <Core/Application.h>
#include <Core/Layer.h>
#include <Renderer.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

namespace Low
{
    Application::Application(const std::string& appName, uint32_t windowWidth, uint32_t windowHeight) :
        m_Name(appName), m_Width(windowWidth), m_Height(windowHeight) {}

	void Application::Init()
	{
        std::vector<const char*> extensions;

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_WindowHandle = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);

        uint32_t extensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        for (uint32_t i = 0; i < extensionCount; i++)
            extensions.push_back(glfwExtensions[i]);

        extensions.push_back("VK_EXT_debug_utils");
        extensionCount++;

        RendererConfig config;
        config.ExtensionCount = extensionCount;
        config.Extensions = extensions.data();
        config.MaxFramesInFlight = 2;

        Renderer::Init(config, m_WindowHandle);

        for (uint32_t i = 0; i < m_Layers.size(); i++)
            m_Layers[i]->Init();
	}

	void Application::Run()
	{
        while (!glfwWindowShouldClose(m_WindowHandle)) 
        {
            for (uint32_t i = 0; i < m_Layers.size(); i++)
                m_Layers[i]->Update();

            glfwPollEvents();
        }

        Stop();
	}

    void Application::Stop()
    {
        glfwDestroyWindow(m_WindowHandle);
        glfwTerminate();

        Renderer::Destroy();
    }
}