#pragma once

#include <string>
#include "d3dHead.h"
#include "d3d/Timer.h"

namespace RainDX
{
    class Application
    {
    public:
        Application(HINSTANCE instance);
        Application(const Application& app) = delete;
        Application& operator=(const Application& app) = delete;
        virtual ~Application();

        virtual bool Init();
        virtual bool Run();
        virtual LRESULT MsgHandler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

    protected:
        virtual void OnResize();
        virtual void Update() = 0;
        virtual void Draw() = 0;
        virtual void OnMouseDown(WPARAM btnState, int x, int y);
        virtual void OnMouseUp(WPARAM btnState, int x, int y);
        virtual void OnMouseMove(WPARAM btnState, int x, int y);


        bool InitWnd();
        bool InitDirectX();
        void CreateDevice();
        void CreateCmd();
        void CreateSwapChain();
        void CheckMsaa();
        void CreateRtvAndDsv();
        void ClearCmdQueue();
        void FrameRate() const;

    public:
        HINSTANCE Inst() const;
        HWND Wnd() const;
        const std::wstring& Title() const;
        float AspectRatio() const;
        ID3D12Resource* CurBuf() const;
        D3D12_CPU_DESCRIPTOR_HANDLE CurBufView() const;
        D3D12_CPU_DESCRIPTOR_HANDLE DepthView() const;

    protected:
        HINSTANCE m_Inst = nullptr;
        HWND m_Wnd = nullptr;
        WNDCLASS m_WndType;
        std::wstring m_Title = L"RainDX Application";
        std::wstring m_Icon = L"misc/RainDX.ico";
        int m_Width = 1400;
        int m_Height = 800;

        Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_Swap;
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;


        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        UINT64 m_CurFence = 0;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CmdQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CmdAlloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CmdList;


        // 后台缓冲区数量
        static constexpr int m_SwapBufCount = 2;
        // 当前后台缓冲区索引
        int m_CurBufIndex = 0;
        // 多个后台缓冲区组成交换链
        Microsoft::WRL::ComPtr<ID3D12Resource> m_SwapBuf[m_SwapBufCount];
        // 深度模板缓冲区
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuf;

        // 渲染目标描述符堆
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        // 深度模板描述符堆
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
        // 常量描述符堆
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CbvHeap = nullptr;

        D3D12_VIEWPORT m_ScreenView;
        D3D12_RECT m_ScissorRect;

        UINT m_RtvSize = 0;
        UINT m_DsvSize = 0;
        UINT m_CbvSrvUavSize = 0;

        D3D_DRIVER_TYPE m_d3d_driver_t = D3D_DRIVER_TYPE_HARDWARE;
        // 后台缓冲区格式
        DXGI_FORMAT m_BufType = DXGI_FORMAT_R8G8B8A8_UNORM;
        // 深度模板缓冲区格式
        DXGI_FORMAT m_DepthBufType = DXGI_FORMAT_D24_UNORM_S8_UINT;
        // 是否支持 MSAA
        bool m_4xMsaaState = false;
        // MSAA 等级
        UINT m_4xMsaaQuality = 0;
        // 计时器
        Timer m_Timer;

        bool m_IsPaused = false;
        bool m_IsMin = false;
        bool m_IsMax = false;
        bool m_IsResizing = false; // are the resize bars being dragged?
        bool m_IsFullScreen = false;

    private:
        static std::wstring m_WndTitle;
        static Application* m_App;

    public:
        static Application* GetApplication();
    };
}
