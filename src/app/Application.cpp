#include "app/Application.h"

RainDX::Application::Application(HINSTANCE instance) : m_Inst(instance)
{
    // redraw window on resize
    m_WndType.style = CS_HREDRAW | CS_VREDRAW;
    m_WndType.lpfnWndProc = MsgHandler;
    m_WndType.cbClsExtra = 0;
    m_WndType.cbWndExtra = 0;
    m_WndType.hInstance = instance;
    m_WndType.hIcon = static_cast<HICON>(LoadImage(nullptr, m_Icon.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
    m_WndType.hCursor = LoadCursor(nullptr, IDC_ARROW);
    m_WndType.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    m_WndType.lpszMenuName = nullptr;
    m_WndType.lpszClassName = m_WndTitle.c_str();
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
    MSG msg { nullptr };
    m_Timer.Reset();
	
    while(msg.message != WM_QUIT)
    {
        if(PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
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
    if( (m_Timer.TotalTime() - timeElapsed) >= 1.0f )
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

LRESULT RainDX::Application::MsgHandler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(wnd, msg, wParam, lParam);
    }
    return 0;
}

std::wstring RainDX::Application::m_WndTitle = L"Application";
