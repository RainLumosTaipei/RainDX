#include "app/Application.h"
#include <cassert>
#include "d3d/DxException.h"

using Microsoft::WRL::ComPtr;

// 初始化 DirectX 相关设置
bool RainDX::Application::InitDirectX()
{
#if defined(DEBUG) || defined(_DEBUG)
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))
        debugController->EnableDebugLayer();
    }
#endif
    CreateDevice();
    CheckMsaa();
    CreateCmd();
    CreateSwapChain();
    CreateRtvAndDsv();

	return true;
}

// 初始化 Rtv 和 Dsv
void RainDX::Application::CreateRtvAndDsv()
{
    m_RtvSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DsvSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// 以及常量缓冲区视图 / 着色器资源视图 / 无序访问视图（CBV/SRV/UAV）
    m_CbvSrvUavSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Rtv
    {
    	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    	rtvHeapDesc.NumDescriptors = m_SwapBufCount;
    	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    	// 该描述符堆关联到GPU, 在单GPU系统中, 通常将其设置为 0
    	rtvHeapDesc.NodeMask = 0;
    	ThrowIfFailed(m_Device->
    		CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_RtvHeap.GetAddressOf())))
    }
	
	// Dsv
    {
    	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    	dsvHeapDesc.NumDescriptors = 1;
    	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    	dsvHeapDesc.NodeMask = 0;
    	ThrowIfFailed(m_Device->
    		CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())))
    }
}

// 初始化 GPU 设备信息
void RainDX::Application::CreateDevice()
{
	// 获取 DXGI 工厂
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_Factory)))

    // 获取设备信息
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr, 
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_Device));
	
    // 在硬件设备创建失败时，尝试使用 WARP 软件设备来创建 Direct3D 12 设备。
    // 这样可以确保在不支持硬件加速的系统上，程序仍然能够正常运行，
    if (FAILED(hardwareResult))
    {
    	// 获取 WARP 设备
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)))
        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_Device)))
    }
	// 同步设备
    ThrowIfFailed(m_Device->
    	CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)))
}

// 初始化命令相关设置
void RainDX::Application::CreateCmd()
{
    // 初始化命令队列
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CmdQueue)))
	}


    // 初始化命令分配器
	{
		ThrowIfFailed(m_Device->
			CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_CmdAlloc.GetAddressOf())))
	}


    // 初始化命令列表
	{
		ThrowIfFailed(m_Device->
			CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_CmdAlloc.Get(), nullptr, 
			IID_PPV_ARGS(m_CmdList.GetAddressOf())))
	}
	
    m_CmdList->Close();
}

// 初始化交换链
void RainDX::Application::CreateSwapChain()
{
    m_Swap.Reset();
	
    DXGI_SWAP_CHAIN_DESC swapDesc;
    swapDesc.BufferDesc.Width = m_Width;
    swapDesc.BufferDesc.Height = m_Height;
    // 帧率 = 60 / 1 = 60
    swapDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapDesc.BufferDesc.Format = m_BufType;
    // 扫描线的排序方式
    swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    // 缓冲区内容的缩放方式
    swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    // 多重采样抗锯齿（MSAA）
    swapDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    swapDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    // 作为渲染目标输出
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // 常见的设置是 2 或 3，分别对应双缓冲或三缓冲机制。
    swapDesc.BufferCount = m_SwapBufCount;
    swapDesc.OutputWindow = m_Wnd;
    // 是否以窗口模式运行
    swapDesc.Windowed = true;
    // 在交换缓冲区后，旧的后台缓冲区内容将被丢弃
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // 允许交换链在窗口模式和全屏模式之间切换
    swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // 创建交换链
    ThrowIfFailed(m_Factory->
    	CreateSwapChain(m_CmdQueue.Get(), &swapDesc, m_Swap.GetAddressOf()))
}

// 设置 MSAA 采样级别
void RainDX::Application::CheckMsaa()
{
    // 查询 Direct3D 12 设备对 4 倍多重采样抗锯齿的支持情况
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = m_BufType;
	// 4倍多重采样抗锯齿(4x MSAA)
    msQualityLevels.SampleCount = 4; 
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(m_Device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)))
	// 采样级别
    m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
}

// 清空命令队列
void RainDX::Application::ClearCmdQueue()
{
	// 同步，等待之前的命令执行完毕
	m_CurFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(m_CmdQueue->Signal(m_Fence.Get(), m_CurFence))

	// Wait until the GPU has completed commands up to this fence point.
	if(m_Fence->GetCompletedValue() < m_CurFence)
	{
		// 事件对象是一种同步原语，可用于线程间或进程间的同步。
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		
		// 当 GPU 完成到 mCurrentFence 这个围栏点的命令时，触发指定的事件对象。
		ThrowIfFailed(m_Fence->SetEventOnCompletion(m_CurFence, eventHandle))
		
		// 用于等待指定的事件对象被触发
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}



// 事件函数：改变窗口大小
void RainDX::Application::OnResize()
{
    assert(m_Device);
	assert(m_Swap);
    assert(m_CmdAlloc);

	// 清空命令
    {
    	// 清空命令队列
    	ClearCmdQueue();
    	// 清空命令列表
    	ThrowIfFailed(m_CmdList->Reset(m_CmdAlloc.Get(), nullptr))
    }
	
	// 重置后台缓冲区和描述符的大小
    {
    	for (int i = 0; i < m_SwapBufCount; ++i)
    		m_SwapBuf[i].Reset();
    	// 调整后台缓冲区的大小
    	ThrowIfFailed(
    		m_Swap->ResizeBuffers(
				m_SwapBufCount, 
				m_Width, m_Height, 
				m_BufType, 
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH))
		// 重置当前缓冲区索引
		m_CurBufIndex = 0;
	
    	// 更新 Rtv
    	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
    	for (UINT i = 0; i < m_SwapBufCount; i++)
    	{
    		ThrowIfFailed(m_Swap->GetBuffer(i, IID_PPV_ARGS(&m_SwapBuf[i])))
			m_Device->CreateRenderTargetView(m_SwapBuf[i].Get(), nullptr, rtvHeapHandle);
    		// ++rtvHandle, 指向下一个 Rtv
    		rtvHeapHandle.Offset(1, m_RtvSize);
    	}
    }

	// 深度模板缓冲区需要销毁后重建
    {
    	m_DepthBuf.Reset();
    	
    	// 新的深度模板缓冲区设置
    	D3D12_RESOURCE_DESC depthDesc;
    	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    	depthDesc.Alignment = 0;
    	depthDesc.Width = m_Width;
    	depthDesc.Height = m_Height;
    	depthDesc.DepthOrArraySize = 1;
    	depthDesc.MipLevels = 1;
    	// we need to create two views to the same resource:
    	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
    	// we need to create the depth buffer resource with a typeless format.  
    	depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    	depthDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    	depthDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    	// 重置值
    	D3D12_CLEAR_VALUE optClear;
    	optClear.Format = m_DepthBufType;
    	optClear.DepthStencil.Depth = 1.0f;
    	optClear.DepthStencil.Stencil = 0;
    	// 在默认堆上创建资源
    	auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    	ThrowIfFailed(m_Device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(m_DepthBuf.GetAddressOf())))

		// 更新 Dsv
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    	dsvDesc.Format = m_DepthBufType;
    	dsvDesc.Texture2D.MipSlice = 0;
    	m_Device->CreateDepthStencilView(m_DepthBuf.Get(), &dsvDesc, DepthView());
    	
    	// 将深度模板缓冲区资源从初始状态转换为深度写入状态
    	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    		m_DepthBuf.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
    	m_CmdList->ResourceBarrier(1, &barrier);
    }

	// 执行重置命令
    {
    	// 执行 resize 命令
    	ThrowIfFailed(m_CmdList->Close())
		ID3D12CommandList* cmdsLists[] = { m_CmdList.Get() };
    	m_CmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    	// 等待完成
    	ClearCmdQueue();
    }

	// 更新屏幕大小和裁剪矩阵
    {
    	m_ScreenView.TopLeftX = 0;
    	m_ScreenView.TopLeftY = 0;
    	m_ScreenView.Width    = static_cast<float>(m_Width);
    	m_ScreenView.Height   = static_cast<float>(m_Height);
    	m_ScreenView.MinDepth = 0.0f;
    	m_ScreenView.MaxDepth = 1.0f;
    	m_ScissorRect = { 0, 0, m_Width, m_Height };
    }
}