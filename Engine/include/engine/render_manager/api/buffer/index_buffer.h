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

#include <cstdint>
#include <memory>
#include <string>

struct D3D12_INDEX_BUFFER_VIEW;
enum   DXGI_FORMAT;

namespace kfe
{
    class KFEDevice;
    class KFEBuffer;

    typedef struct _KFE_INDEX_BUFFER_CREATE_DESC
    {
        KFEDevice*    Device         = nullptr;
        KFEBuffer*    ResourceBuffer = nullptr;
        DXGI_FORMAT   Format         = static_cast<DXGI_FORMAT>(0);
        std::uint64_t OffsetInBytes  = 0u;
    } KFE_INDEX_BUFFER_CREATE_DESC;

    /// <summary>
    /// DirectX 12 Resource Index Wrapper.
    /// </summary>
    class KFE_API KFEIndexBuffer final : public IKFEObject
    {
    public:
         KFEIndexBuffer() noexcept;
        ~KFEIndexBuffer() noexcept;

        KFEIndexBuffer           (const KFEIndexBuffer&) = delete;
        KFEIndexBuffer& operator=(const KFEIndexBuffer&) = delete;

        KFEIndexBuffer           (KFEIndexBuffer&& other) noexcept;
        KFEIndexBuffer& operator=(KFEIndexBuffer&& other) noexcept;

        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        NODISCARD bool Initialize(_In_ const KFE_INDEX_BUFFER_CREATE_DESC& desc);
        
        NODISCARD bool Destroy      () noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD KFEBuffer*       GetBuffer()       noexcept;
        NODISCARD const KFEBuffer* GetBuffer() const noexcept;

        NODISCARD D3D12_INDEX_BUFFER_VIEW GetView() const noexcept;

        NODISCARD DXGI_FORMAT   GetFormat        () const noexcept;
        NODISCARD std::uint64_t GetOffsetInBytes () const noexcept;
        NODISCARD std::uint32_t GetIndexCount    () const noexcept;
        NODISCARD std::uint32_t GetIndexSizeBytes() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
