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
#include "engine/system/exception/base_exception.h"

kfe::BaseException::BaseException(
	const char* file,
	const int   line,
	const char* function,
	const char* message
) : m_szFilePath(file),
	m_nLineNumber(line),
	m_szFunctionName(function)
{
	if (message) m_szErrorMessage = message;
	else m_szErrorMessage		  = "No error message provided";
}

const char* kfe::BaseException::what() const noexcept
{
	if (m_szWhatBuffer.empty())
	{
		m_szWhatBuffer =
			"[BaseException] "	 + m_szErrorMessage				 +
			"\nOn File Path: "	 + m_szFilePath					 +
			"\nAt Line Number: " + std::to_string(m_nLineNumber) +
			"\nFunction: "		 + m_szFunctionName;

	}
	return m_szWhatBuffer.c_str();
}
