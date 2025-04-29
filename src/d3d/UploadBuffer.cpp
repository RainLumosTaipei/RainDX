#include "d3d/UploadBuffer.h"
#include "d3d/DxException.h"

template <typename T>
RainDX::UploadBuffer<T>::UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
    m_IsConst(isConstantBuffer)
{
    // 获取结构大小
    {
        // 获取传入结构的大小
        m_ElementByteSize = sizeof(T);

        // 常量缓冲区必须以 256 字节对齐, 调整为 256 的倍数
        // D3D12_CONSTANT_BUFFER_VIEW_DESC
        if (isConstantBuffer)
            m_ElementByteSize = d3dUtil::ResizeConstBufSize(sizeof(T));
    }

    // 创建上传堆资源
    {
        // 上传堆
        auto prop0 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto desc0 = CD3DX12_RESOURCE_DESC::Buffer(m_ElementByteSize * elementCount);
        // 创建资源
        ThrowIfFailed(device->CreateCommittedResource(
            &prop0,
            D3D12_HEAP_FLAG_NONE,
            &desc0,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_UploadBuf)));
    }

    // 获取 CPU 写指针
    ThrowIfFailed(m_UploadBuf->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedPtr)));
}

template <typename T>
RainDX::UploadBuffer<T>::~UploadBuffer()
{
    // 释放映射
    if (m_UploadBuf != nullptr)
        m_UploadBuf->Unmap(0, nullptr);
    // 清空指针
    m_MappedPtr = nullptr;
}
