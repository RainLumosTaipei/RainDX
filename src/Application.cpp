#include "Application.h"

RainDX::Application::Application(HINSTANCE instance):m_inst(instance)
{
    // redraw window on resize
    wnd_t.style         = CS_HREDRAW | CS_VREDRAW;
    // msg
    wnd_t.lpfnWndProc   = msgHandler; 
    wnd_t.cbClsExtra    = 0;
    wnd_t.cbWndExtra    = 0;
    wnd_t.hInstance     = instance;
    wnd_t.hIcon = (HICON)LoadImage(NULL, L"misc/x.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    // cursor
    wnd_t.hCursor       = LoadCursor(0, IDC_ARROW);
    // bg
    wnd_t.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    // menu
    wnd_t.lpszMenuName  = 0;
    // name
    wnd_t.lpszClassName = wnd_title.c_str();
}

RainDX::Application::~Application()
{
}

bool RainDX::Application::init()
{
    // register window
    if( !RegisterClass(&wnd_t) )
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }
    
    RECT R = { 0, 0, m_width, m_height };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    m_width  = R.right - R.left;
    m_height = R.bottom - R.top;
	
    m_wnd = CreateWindow(wnd_title.c_str(), m_title.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height, 0, 0, m_inst, 0); 
    if( !m_wnd )
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }

    ShowWindow(m_wnd, SW_SHOW);
    UpdateWindow(m_wnd);
    
    return true;
}

bool RainDX::Application::run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

HINSTANCE RainDX::Application::inst() const
{
    return m_inst;
}

HWND RainDX::Application::wnd() const
{
    return m_wnd;
}

const std::wstring& RainDX::Application::title() const
{
    return m_title;
}

LRESULT RainDX::Application::msgHandler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(wnd, msg, wParam, lParam);
    }
    return 0;
}

std::wstring RainDX::Application::wnd_title = L"Application";
