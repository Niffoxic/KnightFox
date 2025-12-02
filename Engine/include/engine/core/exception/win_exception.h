// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : PixelFox (WMG Warwick - Module 2)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once

#include "EngineAPI.h"
#include "base_exception.h"
#include "engine/utils/helpers.h"

#include <windows.h>
#include <string>
#include <string_view>
#include <sal.h>

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
		)
		:	BaseException(file, line, function, "None"),
			m_nLastError(hr)
		{
			LPWSTR buffer = nullptr;

			const DWORD size = ::FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER	|
				FORMAT_MESSAGE_FROM_SYSTEM		|
				FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				m_nLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&buffer),
				0,
				nullptr
			);

			if (size != 0 && buffer != nullptr)
			{
				std::wstring_view view(buffer, size);
				m_szErrorMessage = kfe_helpers::WideToAnsi(view);
				::LocalFree(buffer);
			}
			else
			{
				m_szErrorMessage = "Unrecognized Win32 error.";
			}
		}

		_NODISCARD _Ret_z_ _Ret_valid_ _Check_return_
		const char* what() const noexcept override
		{
			if (m_szWhatBuffer.empty())
			{
				m_szWhatBuffer =
					"[WinException] " + m_szErrorMessage +
					"\nOn File Path: " + m_szFilePath +
					"\nAt Line Number: " + std::to_string(m_nLineNumber) +
					"\nFunction: " + m_szFunctionName;
			}
			return m_szWhatBuffer.c_str();
		}

		_NODISCARD _Check_return_
		DWORD GetErrorCode() const noexcept
		{
			return m_nLastError;
		}

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
