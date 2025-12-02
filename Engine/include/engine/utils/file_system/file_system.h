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
#include <windows.h>
#include <string>
#include <cstdint>
#include <sal.h>

class KFE_API KFEFileSystem
{
public:
     KFEFileSystem() = default;
    ~KFEFileSystem() = default;

    KFEFileSystem(_In_ const KFEFileSystem&) = default;
    KFEFileSystem(_Inout_ KFEFileSystem&&)   = default;

    KFEFileSystem& operator=(const KFEFileSystem&) = default;
    KFEFileSystem& operator=(KFEFileSystem&&)      = default;

    _Check_return_ _NODISCARD
    bool OpenForRead(_In_ const std::string& path);

    _Check_return_ _NODISCARD
    bool OpenForWrite(_In_ const std::string& path);

    void Close();

    _Check_return_ _NODISCARD
    bool ReadBytes(_Out_writes_bytes_all_(size) void*  dest,
                   _In_                         size_t size) const;

    _Check_return_ _NODISCARD
    bool WriteBytes(_In_reads_bytes_(size) const void* data,
                    _In_                   size_t      size) const;

    _Check_return_ _NODISCARD bool ReadUInt32    (_Out_      std::uint32_t& value) const;
    _Check_return_ _NODISCARD bool WriteUInt32   (_In_       std::uint32_t value)  const;
    _Check_return_ _NODISCARD bool ReadString    (_Out_      std::string& outStr)  const;
    _Check_return_ _NODISCARD bool WriteString   (_In_ const std::string& str)     const;
    _Check_return_ _NODISCARD bool WritePlainText(_In_ const std::string& str)     const;

    _NODISCARD _Check_return_ bool          IsOpen     () const;
    _NODISCARD _Check_return_ std::uint64_t GetFileSize() const;

private:
    HANDLE m_fileHandle{ INVALID_HANDLE_VALUE };
    bool   m_bReadMode { false };
};
