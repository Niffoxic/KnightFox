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
#include "engine/core.h"

#include <string>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <sstream>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

namespace kfe::shaders
{
    using BlobPtr = Microsoft::WRL::ComPtr<ID3DBlob>;

    struct SHADER_DESC
    {
        std::string SourcePath;
        std::string EntryPoint{ "main" };
        std::string TargetProfile{ "vs_5_0" };

        bool         EnableDebug{ false };
        bool         WarningsAsErrors{ true };
        std::uint32_t ReservedFlags{ 0u };
    };

    namespace detail
    {
        using CacheMap = std::unordered_map<std::string, BlobPtr>;
        // Single global cache, thanks to C++17 inline variables
        inline CacheMap g_ShaderCache{};

        inline std::string MakeKey(
            const std::string& path,
            const std::string& entry,
            const std::string& target)
        {
            std::ostringstream oss;
            oss << path << '|' << entry << '|' << target;
            return oss.str();
        }

        inline UINT BuildD3DCompileFlags(const SHADER_DESC& desc)
        {
            UINT flags = 0u;

            if (desc.EnableDebug)
            {
                flags |= D3DCOMPILE_DEBUG;
                flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
            }
            if (desc.WarningsAsErrors)
            {
                flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
            }

            flags |= desc.ReservedFlags;
            return flags;
        }

        inline void LogCompileError(ID3DBlob* errorBlob, const std::string& sourcePath)
        {
            if (!errorBlob)
            {
                LOG_ERROR("Shader compile failed for {} with unknown error.", sourcePath);
                return;
            }

            const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
            const size_t msgLen = errorBlob->GetBufferSize();

            std::string errorStr(msg, msg + msgLen);
            LOG_ERROR("Shader compilation error for {}:\n{}", sourcePath, errorStr);
        }
    } // namespace detail

    // ========================================================================
    // Public API (header-only)
    // ========================================================================

    _Use_decl_annotations_
        inline ID3DBlob* GetOrCompile(const SHADER_DESC& desc)
    {
        if (desc.SourcePath.empty())
        {
            LOG_ERROR("kfe::shaders::GetOrCompile: SourcePath is empty.");
            return nullptr;
        }
        if (desc.EntryPoint.empty() || desc.TargetProfile.empty())
        {
            LOG_ERROR("kfe::shaders::GetOrCompile: EntryPoint or TargetProfile is empty for {}.",
                desc.SourcePath);
            return nullptr;
        }

        const std::string key = detail::MakeKey(desc.SourcePath, desc.EntryPoint, desc.TargetProfile);

        // 1) Cache hit
        {
            auto it = detail::g_ShaderCache.find(key);
            if (it != detail::g_ShaderCache.end() && it->second)
            {
                return it->second.Get();
            }
        }

        // 2) Compile
        const std::wstring widePath = kfe_helpers::AnsiToWide(desc.SourcePath);
        const UINT         flags = detail::BuildD3DCompileFlags(desc);

        BlobPtr shaderBlob;
        BlobPtr errorBlob;

        HRESULT hr = D3DCompileFromFile(
            widePath.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            desc.EntryPoint.c_str(),
            desc.TargetProfile.c_str(),
            flags,
            0,
            shaderBlob.GetAddressOf(),
            errorBlob.GetAddressOf()
        );

        if (FAILED(hr))
        {
            detail::LogCompileError(errorBlob.Get(), desc.SourcePath);
            return nullptr;
        }

        detail::g_ShaderCache[key] = shaderBlob;

        LOG_INFO("kfe::shaders: Compiled and cached shader: {} (Entry: {}, Target: {})",
            desc.SourcePath, desc.EntryPoint, desc.TargetProfile);

        return shaderBlob.Get();
    }

    _Use_decl_annotations_
        inline ID3DBlob* GetOrCompile(
            const std::string& sourcePath,
            const std::string& entryPoint = "main",
            const std::string& targetProfile = "vs_5_0")
    {
        SHADER_DESC desc{};
        desc.SourcePath = sourcePath;
        desc.EntryPoint = entryPoint;
        desc.TargetProfile = targetProfile;
        return GetOrCompile(desc);
    }

    _Use_decl_annotations_
        inline bool Remove(
            const std::string& sourcePath,
            const std::string& entryPoint = "main",
            const std::string& targetProfile = "vs_5_0")
    {
        const std::string key = detail::MakeKey(sourcePath, entryPoint, targetProfile);
        auto it = detail::g_ShaderCache.find(key);
        if (it == detail::g_ShaderCache.end())
        {
            return false;
        }

        detail::g_ShaderCache.erase(it);
        return true;
    }

    inline void ClearCache() noexcept
    {
        detail::g_ShaderCache.clear();
    }

    [[nodiscard]]
    inline std::size_t GetCachedCount() noexcept
    {
        return detail::g_ShaderCache.size();
    }

} // namespace kfe::shaders
