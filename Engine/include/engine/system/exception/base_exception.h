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
#include <stdexcept>
#include <string>

namespace kfe
{
	class KFE_API BaseException : public std::exception
	{
	public:
		BaseException(const char* file,
					  const int   line,
					  const char* function,
					  const char* message);

		virtual const char* what() const noexcept override;

	protected:
		std::string			m_szFilePath    {};
		std::string			m_szFunctionName{};
		std::string			m_szErrorMessage{};
		mutable std::string m_szWhatBuffer  {};
		int					m_nLineNumber   { -1 };
	};
}

#define THROW() \
	throw kfe::BaseException(__FILE__, __LINE__, __FUNCTION__)

#define THROW_MSG(msg) \
	throw kfe::BaseException(__FILE__, __LINE__, __FUNCTION__, msg)
