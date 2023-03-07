
#include <Core/Application.h>
#include <Core/Core.h>

#include <ImGui/ImGuiLayer.h>
#include <Rendering/RenderingLayer.h>

#include <iostream>

int main() 
{
    Low::Application application("Lower", 1000, 850);

    application.PushLayer(Low::CreateRef<Low::RenderingLayer>());
    application.PushLayer(Low::CreateRef<Low::ImGuiLayer>());

    application.Init();
    application.Run();

    return 0;
}