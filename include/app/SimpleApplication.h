#pragma once
#include "Application.h"

namespace RainDX
{
    class SimpleApplication : public Application
    {
    public:
        SimpleApplication(HINSTANCE inst);
        ~SimpleApplication() override;


    protected:
        void OnResize() override;
        void Update() override;
        void Draw() override;
    };
}
