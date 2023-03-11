#pragma once

#include <Core/Core.h>
#include <string>
#include <vector>

struct GLFWwindow;

namespace Low
{
	class Layer;
	
	class Application
	{
	public:
		Application(const std::string& appName, uint32_t windowWidth, uint32_t windowHeight);

		void Init();
		void Run();
		void Stop();

		inline void PushLayer(Ref<Layer> layer) { m_Layers.push_back(layer); }

	private:
		std::string m_Name;
		uint32_t m_Width;
		uint32_t m_Height;

		GLFWwindow* m_WindowHandle;
		std::vector<Ref<Layer>> m_Layers;
	};
}