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


struct D3D12_VERTEX_BUFFER_VIEW;

namespace kfe
{
    class KFEDevice;
    class KFEBuffer;

    typedef struct _KFE_VERTEX_BUFFER_CREATE_DESC
    {
        KFEDevice* Device = nullptr;
        KFEBuffer* ResourceBuffer = nullptr;

        std::uint32_t StrideInBytes = 0u;
        std::uint64_t OffsetInBytes = 0u;
        std::string   DebugName{};
    } KFE_VERTEX_BUFFER_CREATE_DESC;

    /// <summary>
    /// Wraps KFEBuffer as a vertex stream and provides a D3D12_VERTEX_BUFFER_VIEW
    /// </summary>
    class KFE_API KFEVertexBuffer final : public IKFEObject
    {
    public:
         KFEVertexBuffer() noexcept;
        ~KFEVertexBuffer() noexcept;

        KFEVertexBuffer           (const KFEVertexBuffer&) = delete;
        KFEVertexBuffer& operator=(const KFEVertexBuffer&) = delete;

        KFEVertexBuffer           (KFEVertexBuffer&& other) noexcept;
        KFEVertexBuffer& operator=(KFEVertexBuffer&& other) noexcept;

        // Inherited via IKFEObject
        NODISCARD std::string GetName       () const noexcept override;
        NODISCARD std::string GetDescription() const noexcept override;

        /// Initializes vertex buffer wrapper from an existing KFEBuffer
        NODISCARD bool Initialize(_In_ const KFE_VERTEX_BUFFER_CREATE_DESC& desc);

        /// Detach from the underlying buffer and reset internal state.
        NODISCARD bool Destroy      () noexcept;
        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD _Ret_maybenull_ KFEBuffer*       GetBuffer()       noexcept;
        NODISCARD _Ret_maybenull_ const KFEBuffer* GetBuffer() const noexcept;

        /// Get the D3D12 vertex buffer view describing this stream.
        NODISCARD D3D12_VERTEX_BUFFER_VIEW GetView() const noexcept;

        NODISCARD std::uint32_t GetStrideInBytes() const noexcept;
        NODISCARD std::uint64_t GetOffsetInBytes() const noexcept;
        NODISCARD std::uint32_t GetVertexCount  () const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace kfe
