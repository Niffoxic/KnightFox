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
#include "engine/utils/helpers.h"
#include <Windows.h>
#include <limits>
#include <algorithm>
#include <cwctype>
#include <cctype>

_Use_decl_annotations_
KFE_API std::string kfe_helpers::ToLowerAscii(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

_Use_decl_annotations_
KFE_API std::wstring kfe_helpers::ToLowerAscii(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return s;
}

_Use_decl_annotations_
KFE_API std::string kfe_helpers::WideToAnsi(std::wstring_view wstr)
{
    if (wstr.empty()) return {};

    if (wstr.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return {};

    const int srcLength = static_cast<int>(wstr.size());

    const int requiredSize = ::WideCharToMultiByte(
        CP_ACP,
        0,
        wstr.data(),
        srcLength,
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (requiredSize <= 0)
        return {};

    std::string result(static_cast<size_t>(requiredSize), '\0');

    const int converted = ::WideCharToMultiByte(
        CP_ACP,
        0,
        wstr.data(),
        srcLength,
        result.data(),
        requiredSize,
        nullptr,
        nullptr
    );

    if (converted <= 0)
        return {};

    if (converted != requiredSize)
        result.resize(static_cast<size_t>(converted));

    return result;
}

_Use_decl_annotations_
KFE_API std::wstring kfe_helpers::AnsiToWide(std::string_view str)
{
    if (str.empty())
        return {};

    if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return {};

    const int srcLength = static_cast<int>(str.size());

    const int requiredSize = ::MultiByteToWideChar(
        CP_ACP,
        0,
        str.data(),
        srcLength,
        nullptr,
        0
    );

    if (requiredSize <= 0)
        return {};

    std::wstring result(static_cast<size_t>(requiredSize), L'\0');

    const int converted = ::MultiByteToWideChar(
        CP_ACP,
        0,
        str.data(),
        srcLength,
        result.data(),
        requiredSize
    );

    if (converted <= 0)
        return {};

    if (converted != requiredSize)
        result.resize(static_cast<size_t>(converted));

    return result;
}

_Use_decl_annotations_
KFE_API bool kfe_helpers::IsPathExists(const std::wstring& path)
{
    DWORD attr = GetFileAttributes(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES);
}

_Use_decl_annotations_
bool kfe_helpers::IsPathExists(const std::string& path)
{
    std::wstring w_path = kfe_helpers::AnsiToWide(path);
    return kfe_helpers::IsPathExists(w_path);
}

_Use_decl_annotations_
bool kfe_helpers::IsDirectory(const std::string& path)
{
    std::wstring w_path = kfe_helpers::AnsiToWide(path);
    DWORD attr = GetFileAttributes(w_path.c_str());

    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

_Use_decl_annotations_
bool kfe_helpers::IsFile(const std::string& path)
{
    std::wstring w_path = kfe_helpers::AnsiToWide(path);
    DWORD attr = GetFileAttributes(w_path.c_str());

    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

_Use_decl_annotations_
bool kfe_helpers::CopyFiles(const std::string& source, const std::string& destination, bool overwrite)
{
    if (!kfe_helpers::IsPathExists(source))
    {
        return false;
    }

    std::wstring srcW = kfe_helpers::AnsiToWide(source);
    std::wstring dstW = kfe_helpers::AnsiToWide(destination);

    return CopyFile(srcW.c_str(), dstW.c_str(), overwrite);
}

_Use_decl_annotations_
bool kfe_helpers::MoveFiles(const std::string& source, const std::string& destination)
{
    if (!kfe_helpers::IsPathExists(source))
    {
        return false;
    }

    std::wstring srcW = kfe_helpers::AnsiToWide(source);
    std::wstring dstW = kfe_helpers::AnsiToWide(destination);

    return MoveFileW(srcW.c_str(), dstW.c_str());
}

_Use_decl_annotations_
DIRECTORY_AND_FILE_NAME kfe_helpers::SplitPathFile(const std::string& fullPath)
{
    size_t lastSlash = fullPath.find_last_of("/\\");
    if (lastSlash == std::string::npos)
    {
        return { "", fullPath };
    }

    return 
    {
        fullPath.substr(0, lastSlash),
        fullPath.substr(lastSlash + 1)
    };
}
