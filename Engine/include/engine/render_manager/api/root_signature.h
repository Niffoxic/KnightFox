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

#include <memory>
#include <string>
#include <cstdint>

struct D3D12_ROOT_PARAMETER;
struct D3D12_STATIC_SAMPLER_DESC;
enum   D3D12_ROOT_SIGNATURE_FLAGS;

namespace kfe
{
    class KFEDevice;

    struct KFE_RG_CREATE_DESC
    {
        KFEDevice* Device{ nullptr };

        const D3D12_ROOT_PARAMETER* RootParameters{ nullptr };
        std::uint32_t                   NumRootParameters{ 0u };

        const D3D12_STATIC_SAMPLER_DESC* StaticSamplers{ nullptr };
        std::uint32_t                    NumStaticSamplers{ 0u };

        D3D12_ROOT_SIGNATURE_FLAGS Flags{};
    };

    class KFE_API KFERootSignature
    {
    public:
        KFERootSignature();
        ~KFERootSignature();

        KFERootSignature(const KFERootSignature&) = delete;
        KFERootSignature& operator=(const KFERootSignature&) = delete;

        KFERootSignature(KFERootSignature&& other) noexcept;
        KFERootSignature& operator=(KFERootSignature&& other) noexcept;

        NODISCARD bool Initialize(_In_ const KFE_RG_CREATE_DESC& desc) noexcept;

        NODISCARD bool InitializeFromSerialized(
            _In_ KFEDevice* device,
            _In_ const void* blob,
            _In_ size_t          blobSize,
            _In_ const wchar_t* debugName = nullptr) noexcept;

        NODISCARD bool InitializeFromFile(
            _In_ KFEDevice* device,
            _In_ const wchar_t* filePath,
            _In_ const wchar_t* debugName = nullptr) noexcept;

        NODISCARD bool Destroy() noexcept;
        void Reset() noexcept;

        NODISCARD bool  IsInitialized() const noexcept;
        NODISCARD void* GetNative()       noexcept;
        NODISCARD const void* GetNative() const noexcept;

        NODISCARD KFE_RG_CREATE_DESC GetCreateDesc() const noexcept;

        void Swap(_Inout_ KFERootSignature& other) noexcept;

        void                  SetDebugName(_In_opt_ const wchar_t* name) noexcept;
        NODISCARD const std::wstring& GetDebugName() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
