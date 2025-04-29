#include "app/SimpleApplication.h"
#include "d3d/DxException.h"
#include <DirectXColors.h>


RainDX::SimpleApplication::SimpleApplication(HINSTANCE inst) : Application(inst)
{
}

RainDX::SimpleApplication::~SimpleApplication()
{
}


void RainDX::SimpleApplication::OnResize()
{
    Application::OnResize();
}

void RainDX::SimpleApplication::Update()
{
}

void RainDX::SimpleApplication::Draw()
{
    ThrowIfFailed(m_CmdAlloc->Reset())
    ThrowIfFailed(m_CmdList->Reset(m_CmdAlloc.Get(), nullptr));

    {
        // 缓冲区变为渲染状态
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(CurBuf(),
                                                            D3D12_RESOURCE_STATE_PRESENT,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_CmdList->ResourceBarrier(1, &barrier);

        m_CmdList->RSSetViewports(1, &m_ScreenView);
        m_CmdList->RSSetScissorRects(1, &m_ScissorRect);

        m_CmdList->ClearRenderTargetView(
            CurBufView(), DirectX::Colors::Wheat, 0, nullptr);
        m_CmdList->ClearDepthStencilView(
            DepthView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // 指定要渲染的缓冲区
        auto curBufView = CurBufView();
        auto depthView = DepthView();
        m_CmdList->OMSetRenderTargets(
            1,
            &curBufView,
            true,
            &depthView);

        // 离开渲染状态
        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(CurBuf(),
                                                             D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                             D3D12_RESOURCE_STATE_PRESENT);
        m_CmdList->ResourceBarrier(1, &barrier1);
    }

    // E_INVALIDARG	一个或多个参数无效	0x80070057
    ThrowIfFailed(m_CmdList->Close());
    ID3D12CommandList* cmdsLists[] = {m_CmdList.Get()};
    m_CmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 交换后台缓冲区索引
    ThrowIfFailed(m_Swap->Present(0, 0))
    m_CurBufIndex = (m_CurBufIndex + 1) % m_SwapBufCount;

    ClearCmdQueue();
}
