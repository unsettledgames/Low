#pragma once

#include <Core/Core.h>
#include <string>
#include <vector>


struct GLFWwindow;

namespace Lower
{
	class Layer;
	
	class Application
	{
	public:
		Application(const std::string& appName, uint32_t windowWidth, uint32_t windowHeight);

		static inline Application* Get() { return s_Application; }

		void Init();
		void Run();
		void Stop();

		inline void PushLayer(Low::Ref<Layer> layer) { m_Layers.push_back(layer); }

		inline GLFWwindow* WindowHandle() { return m_WindowHandle; }

	private:
		void InitRenderer();
		void InitImGui();

	private:
		std::string m_Name;
		uint32_t m_Width;
		uint32_t m_Height;

		GLFWwindow* m_WindowHandle;
		std::vector<Low::Ref<Layer>> m_Layers;

		static Application* s_Application;
	};
}