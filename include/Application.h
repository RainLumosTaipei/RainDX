#pragma once

#include <string>
#include "winHead.h"

namespace RainDX
{
    class Application
    {
    public:
        Application(HINSTANCE instance);
        Application(const Application& app) = delete;
        Application& operator=(const Application& app) = delete;
        virtual ~Application();

    public:
        virtual bool init();
        virtual bool run();

    public:
        HINSTANCE inst() const;
        HWND wnd() const;
        const std::wstring& title() const;
        

    protected:
        HINSTANCE m_inst = nullptr;
        HWND m_wnd = nullptr;
        WNDCLASS wnd_t;
        std::wstring m_title = L"RainDX Application";
        int m_width = 1400;
        int m_height = 800;

    private:
        static LRESULT msgHandler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static std::wstring wnd_title;
    };

}

