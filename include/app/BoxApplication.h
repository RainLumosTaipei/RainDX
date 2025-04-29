#pragma once
#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "Application.h"
#include "d3d/d3dUtil.h"
#include "d3d/MathHelper.h"
#include "d3d/UploadBuffer.h"

namespace RainDX
{
    // 顶点结构体
    struct Vertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT4 Color;
    };

    // 物体常量
    struct ObjConst
    {
        DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
    };

    class BoxApplication : public Application
    {
    public:
        BoxApplication(HINSTANCE inst);
        ~BoxApplication() override;

        bool Init() override;

    protected:
        void OnResize() override;
        void Update() override;
        void Draw() override;
        void OnMouseDown(WPARAM btnState, int x, int y) override;
        void OnMouseMove(WPARAM btnState, int x, int y) override;
        void OnMouseUp(WPARAM btnState, int x, int y) override;

        void CreateCbv();
        void CreateRootSign();
        void BuildShadersAndInputLayout();
        void BuildBoxGeometry();
        void BuildPso();

    private:
        // 根签名
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSign = nullptr;
        // 常量上传缓冲区
        std::unique_ptr<UploadBuffer<ObjConst>> m_ConstBuf = nullptr;

        std::unique_ptr<MeshGeometry> m_BoxGeo = nullptr;
        // 编译的顶点着色器字节码
        Microsoft::WRL::ComPtr<ID3DBlob> m_VertexShader = nullptr;
        // 编译的像素着色器字节码
        Microsoft::WRL::ComPtr<ID3DBlob> m_PixelShader = nullptr;
        // 输入布局
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_Pso = nullptr;

        DirectX::XMFLOAT4X4 m_World = MathHelper::Identity4x4();
        DirectX::XMFLOAT4X4 m_View = MathHelper::Identity4x4();
        DirectX::XMFLOAT4X4 m_Proj = MathHelper::Identity4x4();

        float m_Theta = 1.5f * DirectX::XM_PI;
        float m_Phi = DirectX::XM_PIDIV4;
        float m_Radius = 5.0f;

        POINT m_LastMousePos;
    };
}
