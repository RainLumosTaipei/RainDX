#include "app/Application.h"
#include <WindowsX.h>
#include <cassert>

// 全局函数包装成员函数
LRESULT CALLBACK
MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 使用单例指针调用成员函数
    return RainDX::Application::GetApplication()->MsgHandler(hwnd, msg, wParam, lParam);
}

RainDX::Application::Application(HINSTANCE instance) : m_Inst(instance)
{
    assert(m_App == nullptr);
    m_App = this;
}

RainDX::Application::~Application()
{
}

bool RainDX::Application::Init()
{
    if (!InitWnd()) return false;
    if (!InitDirectX()) return false;

    OnResize();

    return true;
}

bool RainDX::Application::Run()
{
    MSG msg{nullptr};
    m_Timer.Reset();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            m_Timer.Tick();
            FrameRate();
            Update();
            Draw();
        }
    }

    return true;
}

void RainDX::Application::FrameRate() const
{
    // 每秒帧数
    static int frameCnt = 0;
    static float timeElapsed = 0.0f;

    ++frameCnt;

    // Compute averages over one second period.
    if ((m_Timer.TotalTime() - timeElapsed) >= 1.0f)
    {
        float fps = static_cast<float>(frameCnt); // fps = frameCnt / 1
        float tpf = 1000.0f / fps;

        std::wstring fpsStr = std::to_wstring(fps);
        std::wstring tpfStr = std::to_wstring(tpf);
        std::wstring windowText = m_Title +
            L"    fps: " + fpsStr +
            L"    tpf: " + tpfStr + L" ms";
        SetWindowText(m_Wnd, windowText.c_str());

        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}


bool RainDX::Application::InitWnd()
{
    {
        // redraw window on resize
        m_WndType.style = CS_HREDRAW | CS_VREDRAW;
        m_WndType.lpfnWndProc = MsgProc;
        m_WndType.cbClsExtra = 0;
        m_WndType.cbWndExtra = 0;
        m_WndType.hInstance = m_Inst;
        m_WndType.hIcon = static_cast<HICON>(LoadImage(nullptr, m_Icon.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
        m_WndType.hCursor = LoadCursor(nullptr, IDC_ARROW);
        m_WndType.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        m_WndType.lpszMenuName = nullptr;
        m_WndType.lpszClassName = m_WndTitle.c_str();
    }
    // register window
    if (!RegisterClass(&m_WndType))
    {
        MessageBox(nullptr, L"RegisterClass Failed.", nullptr, 0);
        return false;
    }

    RECT R = {0, 0, m_Width, m_Height};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    m_Width = R.right - R.left;
    m_Height = R.bottom - R.top;

    m_Wnd = CreateWindow(m_WndTitle.c_str(), m_Title.c_str(),
                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_Width, m_Height, 0, 0, m_Inst, 0);
    if (!m_Wnd)
    {
        MessageBox(nullptr, L"CreateWindow Failed.", nullptr, 0);
        return false;
    }

    ShowWindow(m_Wnd, SW_SHOW);
    UpdateWindow(m_Wnd);

    return true;
}

HINSTANCE RainDX::Application::Inst() const
{
    return m_Inst;
}

HWND RainDX::Application::Wnd() const
{
    return m_Wnd;
}

const std::wstring& RainDX::Application::Title() const
{
    return m_Title;
}

float RainDX::Application::AspectRatio() const
{
    return static_cast<float>(m_Width) / m_Height;
}

ID3D12Resource* RainDX::Application::CurBuf() const
{
    return m_SwapBuf[m_CurBufIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE RainDX::Application::CurBufView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_CurBufIndex,
        m_RtvSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE RainDX::Application::DepthView() const
{
    return m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

RainDX::Application* RainDX::Application::GetApplication()
{
    return m_App;
}

LRESULT RainDX::Application::MsgHandler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // WM_ACTIVATE is sent when the window is activated or deactivated.  
    // We pause the game when the window is deactivated and unpause it 
    // when it becomes active.  
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            m_IsPaused = true;
            m_Timer.Stop();
        }
        else
        {
            m_IsPaused = false;
            m_Timer.Start();
        }
        return 0;

    // WM_SIZE is sent when the user resizes the window.  
    case WM_SIZE:
        // Save the new client area dimensions.
        m_Width = LOWORD(lParam);
        m_Height = HIWORD(lParam);
        if (m_Device)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                m_IsPaused = true;
                m_IsMin = true;
                m_IsMax = false;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                m_IsPaused = false;
                m_IsMin = false;
                m_IsMax = true;
                OnResize();
            }
            else if (wParam == SIZE_RESTORED)
            {
                // Restoring from minimized state?
                if (m_IsMin)
                {
                    m_IsPaused = false;
                    m_IsMin = false;
                    OnResize();
                }

                // Restoring from maximized state?
                else if (m_IsMax)
                {
                    m_IsPaused = false;
                    m_IsMax = false;
                    OnResize();
                }
                else if (m_IsResizing)
                {
                    // If user is dragging the resize bars, we do not resize 
                    // the buffers here because as the user continuously 
                    // drags the resize bars, a stream of WM_SIZE messages are
                    // sent to the window, and it would be pointless (and slow)
                    // to resize for each WM_SIZE message received from dragging
                    // the resize bars.  So instead, we reset after the user is 
                    // done resizing the window and releases the resize bars, which 
                    // sends a WM_EXITSIZEMOVE message.
                }
                else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
                {
                    OnResize();
                }
            }
        }
        return 0;

    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        m_IsPaused = true;
        m_IsResizing = true;
        m_Timer.Stop();
        return 0;

    // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        m_IsPaused = false;
        m_IsResizing = false;
        m_Timer.Start();
        OnResize();
        return 0;

    // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    // The WM_MENUCHAR message is sent when a menu is active and the user presses 
    // a key that does not correspond to any mnemonic or accelerator key. 
    case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

    // Catch this message so to prevent the window from becoming too small.
    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
        return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if (static_cast<int>(wParam) == VK_F2)
            m_4xMsaaState = !m_4xMsaaState;

        return 0;
    }

    return DefWindowProc(wnd, msg, wParam, lParam);
}

std::wstring RainDX::Application::m_WndTitle = L"Application";

RainDX::Application* RainDX::Application::m_App = nullptr;
