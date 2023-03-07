#include <Core/Application.h>
#include <Core/Layer.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

namespace Low
{
    Application::Application(const std::string& appName, uint32_t windowWidth, uint32_t windowHeight) :
        m_Name(appName), m_Width(windowWidth), m_Height(windowHeight) {}

	void Application::Init()
	{
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_WindowHandler = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::cout << extensionCount << " extensions supported\n";

        for (uint32_t i = 0; i < m_Layers.size(); i++)
            m_Layers[i]->Init();
	}

	void Application::Run()
	{
        while (!glfwWindowShouldClose(m_WindowHandler)) 
        {
            for (uint32_t i = 0; i < m_Layers.size(); i++)
                m_Layers[i]->Update();

            glfwPollEvents();
        }

        glfwDestroyWindow(m_WindowHandler);
        glfwTerminate();
	}
}