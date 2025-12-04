// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : KnightFox (WMG Warwick - Module 2 WM9M2:Computer Graphics)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */
#pragma once

#include "EngineAPI.h"
#include "engine/core.h"
#include "base_exception.h"

#include <string>

namespace kfe
{
    class KFE_API DxException final : public BaseException
    {
    public:
        DxException(
            _In_z_ const char* file,
            _In_   int         line,
            _In_z_ const char* function,
            _In_   HRESULT     hr
        ) noexcept;

        NODISCARD const char* what    () const noexcept override;
        NODISCARD HRESULT GetErrorCode() const noexcept;

    private:
        void BuildErrorMessage() noexcept;

    private:
        HRESULT m_nErrorCode{};
    };
} // namespace kfe

#define THROW_DX_IF_FAILS(_hr_expr)                                  \
    do                                                               \
    {                                                                \
        const HRESULT _hr_internal_ = (_hr_expr);                    \
        if (FAILED(_hr_internal_))                                   \
        {                                                            \
            throw kfe::DxException(                                  \
                __FILE__,                                            \
                __LINE__,                                            \
                __FUNCTION__,                                        \
                _hr_internal_);                                      \
        }                                                            \
    } while (0)
