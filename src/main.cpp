#include <iostream>
#include <memory>
#include "app/SimpleApplication.h"
#include "d3d/DxException.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                   PSTR cmdLine, int showCmd)
{
    auto app = std::make_unique<RainDX::SimpleApplication>(hInstance);

    try
    {
        app->Init();
        app->Run();
    }
    catch (RainDX::DxException& e)
    {
        std::wcout << e.ToString();
        return 0;
    }


    return 0;
}
