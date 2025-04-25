#include <memory>
#include "Application.h"
#include "winHead.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
                   PSTR cmdLine, int showCmd)
{
    std::unique_ptr<RainDX::Application> app = std::make_unique<RainDX::Application>(hInstance);
    app->init();
    app->run();
    
    return 0;
}
