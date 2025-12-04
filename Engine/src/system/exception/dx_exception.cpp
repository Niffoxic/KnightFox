// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#include "pch.h"
#include "engine/system/exception/dx_exception.h"

#include <comdef.h> 

_Use_decl_annotations_
kfe::DxException::DxException(
    const char* file,
    int         line,
    const char* function,
    HRESULT     hr
) noexcept
    : BaseException(file, line, function, "DirectX call failed")
    , m_nErrorCode(hr)
{
    BuildErrorMessage();
}

_Use_decl_annotations_
const char* kfe::DxException::what() const noexcept
{
    if (m_szWhatBuffer.empty())
    {
        std::ostringstream oss;

        oss << "[DxException] " << m_szErrorMessage
            << "\n  HRESULT : 0x"
            << std::hex << std::uppercase
            << std::setw(8) << std::setfill('0')
            << static_cast<unsigned long>(m_nErrorCode)
            << std::dec
            << "\n  File    : " << m_szFilePath
            << "\n  Line    : " << m_nLineNumber
            << "\n  Function: " << m_szFunctionName;

        m_szWhatBuffer = oss.str();
    }
    return m_szWhatBuffer.c_str();
}

_Use_decl_annotations_
HRESULT kfe::DxException::GetErrorCode() const noexcept
{
    return m_nErrorCode;
}

void kfe::DxException::BuildErrorMessage() noexcept
{
    _com_error err(m_nErrorCode);
    const wchar_t* wideMsg = err.ErrorMessage();

    if (wideMsg != nullptr)
    {
        int requiredSize = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            wideMsg,
            -1,
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (requiredSize > 0)
        {
            std::string utf8(static_cast<size_t>(requiredSize), '\0');

            ::WideCharToMultiByte(
                CP_UTF8,
                0,
                wideMsg,
                -1,
                utf8.data(),
                requiredSize,
                nullptr,
                nullptr
            );

            if (!utf8.empty() && utf8.back() == '\0')
                utf8.pop_back();

            m_szErrorMessage = utf8;
            return;
        }
    }
    m_szErrorMessage = "Unknown DirectX error.";
}
