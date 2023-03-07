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
		std::cout << "Rendering update" << std::endl;
	}
}