#pragma once
#include <intsafe.h>
#include <string>
#include "app/Application.h"

namespace RainDX
{
    class DxException
    {
    public:
        DxException() = default;
        DxException(HRESULT hr, const std::wstring& func, const std::wstring& file, int line);
        std::wstring ToString() const;

    private:
        HRESULT code = S_OK;
        std::wstring func;
        std::wstring file;
        int line = -1;
    };
}


inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return {buffer};
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw RainDX::DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
