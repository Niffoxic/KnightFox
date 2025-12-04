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

#include <string>
#include <cstdint>
#include <memory>

class KFE_API KFEFileSystem
{
public:
     KFEFileSystem();
    ~KFEFileSystem();

    KFEFileSystem(_In_ const KFEFileSystem&) = delete;
    KFEFileSystem(_Inout_ KFEFileSystem&&)   = delete;

    KFEFileSystem& operator=(const KFEFileSystem&) = delete;
    KFEFileSystem& operator=(KFEFileSystem&&)      = delete;

    NODISCARD bool OpenForRead (_In_ const std::string& path);
    NODISCARD bool OpenForWrite(_In_ const std::string& path);

    void Close();
    NODISCARD bool ReadBytes(_Out_writes_bytes_all_(size) void*  dest,
                             _In_                         size_t size) const;
    NODISCARD bool WriteBytes(_In_reads_bytes_(size) const void* data,
                             _In_                    size_t      size) const;

    NODISCARD bool ReadUInt32    (_Out_      std::uint32_t& value) const;
    NODISCARD bool WriteUInt32   (_In_       std::uint32_t value ) const;
    NODISCARD bool ReadString    (_Out_      std::string& outStr ) const;
    NODISCARD bool WriteString   (_In_ const std::string& str    ) const;
    NODISCARD bool WritePlainText(_In_ const std::string& str    ) const;

    NODISCARD bool          IsOpen     () const;
    NODISCARD std::uint64_t GetFileSize() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    HANDLE m_fileHandle{ INVALID_HANDLE_VALUE };
    bool   m_bReadMode { false };
};
