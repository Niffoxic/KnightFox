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
#include "engine/system/exception/win_exception.h"

_Use_decl_annotations_
kfe::WinException::WinException(
	const char* file,
	int		    line,
	const char* function,
	DWORD	    hr
) 
	: BaseException(file, line, function, "None"),
	m_nLastError(hr)
{
	LPWSTR buffer = nullptr;

	const DWORD size = ::FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
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

_Use_decl_annotations_
const char* kfe::WinException::what() const noexcept
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

_Use_decl_annotations_
DWORD kfe::WinException::GetErrorCode() const noexcept
{
	return m_nLastError;
}