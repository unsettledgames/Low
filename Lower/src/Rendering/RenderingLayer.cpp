#include <Renderer.h>
#include <GLFW/glfw3.h>
#include <Rendering/RenderingLayer.h>
#include <iostream>

namespace Low
{
	RenderingLayer::RenderingLayer() : Layer("RenderingLayer") {}

	void RenderingLayer::Init()
	{

	}

	void RenderingLayer::Update()
	{
		glfwPollEvents();
		Renderer::DrawFrame();
	}
}