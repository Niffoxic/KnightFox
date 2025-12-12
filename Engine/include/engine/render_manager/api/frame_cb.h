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

#include "buffer/constant_buffer.h"
#include "buffer/buffer.h"

struct ID3D12Fence;

namespace kfe
{
    /// <summary>
    /// Frame constant buffer pool guarded by a fence
    /// </summary>
    class KFEFrameConstantBuffer
    {
    public:
        explicit KFEFrameConstantBuffer(std::uint16_t counts);
        ~KFEFrameConstantBuffer();

        KFEFrameConstantBuffer(const KFEFrameConstantBuffer&) = delete;
        KFEFrameConstantBuffer& operator=(const KFEFrameConstantBuffer&) = delete;

        KFEFrameConstantBuffer(KFEFrameConstantBuffer&& other) noexcept;
        KFEFrameConstantBuffer& operator=(KFEFrameConstantBuffer&& other) noexcept;

        NODISCARD bool Initialize(const KFE_CREATE_BUFFER_DESC& buffer,
                                  const KFE_CONSTANT_BUFFER_CREATE_DESC& view);

        /// Get a buffer that is safe to write into
        NODISCARD KFEBuffer* Get() const noexcept;
        NODISCARD KFEConstantBuffer* GetView(_In_ const KFEBuffer* buffer) const noexcept;

        /// Mark a buffer as "in use" until the given fence value completes.
        void Mark(_In_ KFEBuffer* buffer,
            _In_ ID3D12Fence* fence,
            std::uint64_t fenceValue) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
