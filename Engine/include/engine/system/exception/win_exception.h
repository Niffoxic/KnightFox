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
#include "base_exception.h"
#include "engine/utils/helpers.h"
#include "engine/core.h"

#include <string>

namespace kfe
{
	class KFE_API WinException final : public BaseException
	{
	public:
		WinException(
			_In_z_ const char* file,
			_In_   int		   line,
			_In_z_ const char* function,
			_In_   DWORD	   hr = ::GetLastError()
		);

		NODISCARD const char* what		  () const noexcept override;
		NODISCARD DWORD		  GetErrorCode() const noexcept;

	private:
		DWORD m_nLastError{};
	};
} // namespace kfe

#define THROW_WIN() \
    throw kfe::WinException(__FILE__, __LINE__, __FUNCTION__)

#define THROW_WIN_IF_FAILS(_hr_expr) \
    do { HRESULT _hr_internal = (_hr_expr); if (FAILED(_hr_internal)) { \
        throw kfe::WinException(__FILE__, __LINE__, __FUNCTION__, static_cast<DWORD>(_hr_internal)); \
    } } while(0)
