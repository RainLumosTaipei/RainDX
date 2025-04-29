#pragma once
#include "d3dHead.h"
#include "d3dUtil.h"

namespace RainDX
{
    // 上传缓冲区
    template <typename T>
    class UploadBuffer
    {
    public:
        UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer);
        UploadBuffer(const UploadBuffer& rhs) = delete;
        UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
        ~UploadBuffer();


        ID3D12Resource* Resource() const
        {
            return m_UploadBuf.Get();
        }

        // CPU 更新数据
        void CopyData(int elementIndex, const T& data)
        {
            memcpy(&m_MappedPtr[elementIndex * m_ElementByteSize], &data, sizeof(T));
        }

    private:
        // 上传堆
        Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadBuf;
        // CPU 写指针，向其写入可以更新 GPU 侧的缓冲区
        BYTE* m_MappedPtr = nullptr;
        // 每个结构的大小
        UINT m_ElementByteSize = 0;
        // 是否为常量缓冲区
        bool m_IsConst = false;
    };
}
