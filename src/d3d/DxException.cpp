#include "d3d/DxException.h"
#include "comdef.h"

RainDX::DxException::DxException
(HRESULT hr, const std::wstring& func, const std::wstring& file, int line):
    code(hr), func(func), file(file), line(line)
{
}

std::wstring RainDX::DxException::ToString() const
{
    _com_error err(code);
    std::wstring msg = err.ErrorMessage();

    return func + L" failed in " + file + L"; line " + std::to_wstring(line) + L"; error: " + msg;
}
