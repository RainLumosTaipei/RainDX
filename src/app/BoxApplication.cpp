#include "app/BoxApplication.h"
#include <array>
#include <d3dcompiler.h>
#include <DirectXColors.h>

#include "d3d/DxException.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

RainDX::BoxApplication::BoxApplication(HINSTANCE hInstance) : Application(hInstance)
{
}

RainDX::BoxApplication::~BoxApplication()
{
}

bool RainDX::BoxApplication::Init()
{
    if (!Application::Init())
        return false;

    // 重置命令列表
    ThrowIfFailed(m_CmdList->Reset(m_CmdAlloc.Get(), nullptr));
    
    CreateCbv();
    CreateRootSign();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPso();

    // 执行完毕
    {
        ThrowIfFailed(m_CmdList->Close());
        ID3D12CommandList* cmdsLists[] = {m_CmdList.Get()};
        m_CmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
        ClearCmdQueue();
    }

    return true;
}

void RainDX::BoxApplication::OnResize()
{
    Application::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&m_Proj, P);
}

// 每帧更新计算观察矩阵
void RainDX::BoxApplication::Update()
{
    // 从球坐标系转换到直角坐标系
    float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
    float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
    float y = m_Radius * cosf(m_Phi);

    // 观察矩阵
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_View, view);

    XMMATRIX world = XMLoadFloat4x4(&m_World);
    XMMATRIX proj = XMLoadFloat4x4(&m_Proj);
    XMMATRIX worldViewProj = world * view * proj;

    // 更新常量缓冲区的世界矩阵
    ObjConst objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    objConstants.Time = m_Timer.TotalTime();
    m_ConstBuf->CopyData(0, objConstants);
}

// 绘制指令
void RainDX::BoxApplication::Draw()
{
    // 清空
    {
        ThrowIfFailed(m_CmdAlloc->Reset());
        ThrowIfFailed(m_CmdList->Reset(m_CmdAlloc.Get(), m_Pso.Get()));

        m_CmdList->RSSetViewports(1, &m_ScreenView);
        m_CmdList->RSSetScissorRects(1, &m_ScissorRect);
        
        auto br0 = CD3DX12_RESOURCE_BARRIER::Transition(
            CurBuf(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_CmdList->ResourceBarrier(1, &br0);
    
        m_CmdList->ClearRenderTargetView(CurBufView(), Colors::Wheat, 0, nullptr);
        m_CmdList->ClearDepthStencilView(DepthView(),
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0,nullptr);
    }
    
    auto cur = CurBufView();
    auto dept = DepthView();
    m_CmdList->OMSetRenderTargets(1, &cur, true, &dept);

    // 设置描述符堆
    ID3D12DescriptorHeap* descriptorHeaps[] = {m_CbvHeap.Get()};
    m_CmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    // 设置根签名
    m_CmdList->SetGraphicsRootSignature(m_RootSign.Get());

    auto vertex = m_BoxGeo->VertexBufferView();
    auto index = m_BoxGeo->IndexBufferView();
    // 设置顶点缓冲区
    m_CmdList->IASetVertexBuffers(0, 1, &vertex);
    // 设置索引缓冲区
    m_CmdList->IASetIndexBuffer(&index);
    m_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 将第一个寄存器绑定到常量缓冲区
    m_CmdList->SetGraphicsRootDescriptorTable(0, m_CbvHeap->GetGPUDescriptorHandleForHeapStart());
    // 根据索引绘制物体
    m_CmdList->DrawIndexedInstanced(
        m_BoxGeo->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);

    // 转为呈现状态
    auto br1 = CD3DX12_RESOURCE_BARRIER::Transition(
        CurBuf(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_CmdList->ResourceBarrier(1, &br1);

    // 执行
    {
        ThrowIfFailed(m_CmdList->Close());
        ID3D12CommandList* cmdsLists[] = {m_CmdList.Get()};
        m_CmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    }

    ThrowIfFailed(m_Swap->Present(0, 0));
    m_CurBufIndex = (m_CurBufIndex + 1) % m_SwapBufCount;
    
    ClearCmdQueue();
}

void RainDX::BoxApplication::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_LastMousePos.x = x;
    m_LastMousePos.y = y;

    SetCapture(m_Wnd);
}

void RainDX::BoxApplication::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void RainDX::BoxApplication::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));

        // Update angles based on input to orbit camera around box.
        m_Theta += dx;
        m_Phi += dy;

        // Restrict the angle mPhi.
        m_Phi = MathHelper::Clamp(m_Phi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f * static_cast<float>(x - m_LastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - m_LastMousePos.y);

        // Update the camera radius based on input.
        m_Radius += dx - dy;

        // Restrict the radius.
        m_Radius = MathHelper::Clamp(m_Radius, 3.0f, 15.0f);
    }

    m_LastMousePos.x = x;
    m_LastMousePos.y = y;
}


// 创建常量缓冲区
void RainDX::BoxApplication::CreateCbv()
{
    // 创建常量缓冲区描述符堆
    {
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
        cbvHeapDesc.NumDescriptors = 1;
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cbvHeapDesc.NodeMask = 0;
        ThrowIfFailed(m_Device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CbvHeap)));
    }

    // 利用上传堆创建常量缓冲区
    m_ConstBuf = std::make_unique<UploadBuffer<ObjConst>>(m_Device.Get(), 1, true);

    // 创建常量描述符
    {
        // 常量缓冲区起始地址
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_ConstBuf->Resource()->GetGPUVirtualAddress();
        // 第一个物体的偏移量
        int boxCBufIndex = 0;
        UINT objCBByteSize = d3dUtil::ResizeConstBufSize(sizeof(ObjConst));
        cbAddress += boxCBufIndex * objCBByteSize;

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = d3dUtil::ResizeConstBufSize(sizeof(ObjConst));
        // 创建常量描述符
        m_Device->CreateConstantBufferView(
            &cbvDesc,
            m_CbvHeap->GetCPUDescriptorHandleForHeapStart());
    }

}

// 创建根签名
void RainDX::BoxApplication::CreateRootSign()
{
    // 根参数数组
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];
    // 描述符表
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    // 一个常量描述符，且对应于第一个基准寄存器
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    // 作为第一个根参数
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    // 根签名描述
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // 序列化根签名
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
    }
    ThrowIfFailed(hr);

    // 创建根签名
    ThrowIfFailed(m_Device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_RootSign)));
}

// 编译着色器和输入布局
void RainDX::BoxApplication::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;

    m_VertexShader = d3dUtil::CompileShader(L"shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    m_PixelShader = d3dUtil::CompileShader(L"shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    m_InputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

// 创建顶点和索引缓冲区
void RainDX::BoxApplication::BuildBoxGeometry()
{
    // 运行时在内存中分配顶点数据
    std::array<Vertex, 8> vertices =
    {
        Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
        Vertex({XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
        Vertex({XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
        Vertex({XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
        Vertex({XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)}),
        Vertex({XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)}),
        Vertex({XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)}),
        Vertex({XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)})
    };

    // uint16_t
    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    constexpr UINT vbByteSize = static_cast<UINT>(vertices.size()) * sizeof(Vertex);
    constexpr UINT ibByteSize = static_cast<UINT>(indices.size()) * sizeof(std::uint16_t);

    m_BoxGeo = std::make_unique<MeshGeometry>();
    m_BoxGeo->Name = "boxGeo";
    m_BoxGeo->VertexByteStride = sizeof(Vertex);
    m_BoxGeo->VertexBufferByteSize = vbByteSize;
    m_BoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    m_BoxGeo->IndexBufferByteSize = ibByteSize;
    
    // 复制数据
    {
        // 分配空间
        ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_BoxGeo->VertexBufferCPU));
        // 复制数据
        CopyMemory(m_BoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

        ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_BoxGeo->IndexBufferCPU));
        CopyMemory(m_BoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

        m_BoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device.Get(),
                                                                m_CmdList.Get(), vertices.data(), vbByteSize,
                                                                m_BoxGeo->VertexBufferUploader);

        m_BoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_Device.Get(),
                                                               m_CmdList.Get(), indices.data(), ibByteSize,
                                                               m_BoxGeo->IndexBufferUploader);
    }
    
    // 子物体
    SubmeshGeometry submesh;
    // 大小为索引数组
    submesh.IndexCount = static_cast<UINT>(indices.size());
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    // 记录对应关系
    m_BoxGeo->DrawArgs["box"] = submesh;
}

// 创建流水线描述
void RainDX::BoxApplication::BuildPso()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {m_InputLayout.data(), static_cast<UINT>(m_InputLayout.size())};
    psoDesc.pRootSignature = m_RootSign.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_VertexShader->GetBufferPointer()),
        m_VertexShader->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_PixelShader->GetBufferPointer()),
        m_PixelShader->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_BufType;
    psoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = m_DepthBufType;
    // 绑定到流水线
    ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_Pso)));
}


