#pragma once

#include <Core/Core.h>
#include <Resources/Mesh.h>
#include <Resources/MaterialInstance.h>

#include <string>
#include <vector>


struct GLFWwindow;

using namespace Low;

namespace Lower
{
	class Application
	{
	public:
		Application(const std::string& appName, uint32_t windowWidth, uint32_t windowHeight);

		static inline Application* Get() { return s_Application; }

		void Init();
		void Run();
		void Stop();

		inline GLFWwindow* WindowHandle() { return m_WindowHandle; }

	private:
		void InitRenderer();
		void InitImGui();

	private:
		std::string m_Name;
		uint32_t m_Width;
		uint32_t m_Height;

		GLFWwindow* m_WindowHandle;

		std::vector<Ref<Mesh>> m_Meshes;
		std::vector<Ref<MaterialInstance>> m_Materials;

		static Application* s_Application;
	};
}