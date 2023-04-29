
#include <Core/Application.h>
#include <Core/Core.h>

#include <iostream>

int main() 
{
    Lower::Application application("Lower", 1000, 850);

    application.Init();
    application.Run();

    return 0;
}