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
#include "engine/render_manager/api/frame_cb.h"

#include "engine/utils/logger.h"

#include <vector>
#include <algorithm>

#include <d3d12.h>

#pragma region Impl_Definition

class kfe::KFEFrameConstantBuffer::Impl
{
public:
    struct Slot
    {
        std::unique_ptr<KFEBuffer>         OwnedBuffer;
        std::unique_ptr<KFEConstantBuffer> OwnedView;

        //~ Free Buffer and View
        KFEBuffer* Buffer       = nullptr;
        KFEConstantBuffer* View = nullptr;

        ID3D12Fence*       Fence      = nullptr;
        std::uint64_t      FenceValue = 0u;
        bool               InUse      = false;
    };

    explicit Impl(std::uint16_t count) noexcept;

    NODISCARD bool Initialize(const KFE_CREATE_BUFFER_DESC&          bufferDesc,
                              const KFE_CONSTANT_BUFFER_CREATE_DESC& viewDesc);

    KFEBuffer* Get() const noexcept;

    KFEConstantBuffer* GetView(const KFEBuffer* buffer) const noexcept;

    void Mark(KFEBuffer*    buffer,
              ID3D12Fence*  fence,
              std::uint64_t fenceValue) noexcept;

private:
    mutable std::vector<Slot>     m_slots;
    std::uint16_t         m_count;
    mutable std::uint16_t m_cursor;
};

#pragma endregion

#pragma region Frame_Body

kfe::KFEFrameConstantBuffer::KFEFrameConstantBuffer(std::uint16_t counts)
    : m_impl(std::make_unique<Impl>(counts))
{
}

kfe::KFEFrameConstantBuffer::~KFEFrameConstantBuffer() = default;

kfe::KFEFrameConstantBuffer::KFEFrameConstantBuffer(KFEFrameConstantBuffer&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

kfe::KFEFrameConstantBuffer& kfe::KFEFrameConstantBuffer::operator=(KFEFrameConstantBuffer&& other) noexcept
{
    if (this != &other)
    {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

_Use_decl_annotations_
bool kfe::KFEFrameConstantBuffer::Initialize(const KFE_CREATE_BUFFER_DESC& buffer,
    const KFE_CONSTANT_BUFFER_CREATE_DESC& view)
{
    if (!m_impl)
    {
        return false;
    }

    return m_impl->Initialize(buffer, view);
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEFrameConstantBuffer::Get() const noexcept
{
    if (!m_impl)
    {
        return nullptr;
    }

    return m_impl->Get();
}

_Use_decl_annotations_
kfe::KFEConstantBuffer* kfe::KFEFrameConstantBuffer::GetView(const KFEBuffer* buffer) const noexcept
{
    if (!m_impl)
    {
        return nullptr;
    }

    return m_impl->GetView(buffer);
}

_Use_decl_annotations_
void kfe::KFEFrameConstantBuffer::Mark(KFEBuffer* buffer,
    ID3D12Fence* fence,
    std::uint64_t fenceValue) noexcept
{
    if (!m_impl)
    {
        return;
    }

    m_impl->Mark(buffer, fence, fenceValue);
}

#pragma endregion

#pragma region Impl_Body

kfe::KFEFrameConstantBuffer::Impl::Impl(std::uint16_t count) noexcept
    : m_slots(count)
    , m_count(count)
    , m_cursor(0)
{}

_Use_decl_annotations_
bool kfe::KFEFrameConstantBuffer::Impl::Initialize(const KFE_CREATE_BUFFER_DESC& bufferDesc,
    const KFE_CONSTANT_BUFFER_CREATE_DESC& viewDesc)
{
    if (m_slots.empty())
    {
        LOG_ERROR("KFEFrameConstantBuffer::Initialize - no slots allocated.");
        return false;
    }

    for (std::uint16_t i = 0; i < m_count; ++i)
    {
        auto& slot = m_slots[i];

        slot.OwnedBuffer.reset();
        slot.OwnedView  .reset();
        slot.Buffer     = nullptr;
        slot.View       = nullptr;
        slot.Fence      = nullptr;
        slot.FenceValue = 0;
        slot.InUse      = false;

        slot.OwnedBuffer = std::make_unique<KFEBuffer>();
        slot.OwnedView   = std::make_unique<KFEConstantBuffer>();

        if (!slot.OwnedBuffer || !slot.OwnedView)
        {
            LOG_ERROR("failed to allocate buffer and view objects.");
            return false;
        }

        //~ Create the buffer
        KFE_CREATE_BUFFER_DESC bufDesc = bufferDesc;
        if (!slot.OwnedBuffer->Initialize(bufDesc))
        {
            LOG_ERROR("failed to initialize constant buffer for slot {}.", i);
            return false;
        }

        //~ Create the constant buffer view
        KFE_CONSTANT_BUFFER_CREATE_DESC cbvDesc = viewDesc;
        cbvDesc.ResourceBuffer = slot.OwnedBuffer.get();

        if (!slot.OwnedView->Initialize(cbvDesc))
        {
            LOG_ERROR("KFEFrameConstantBuffer::Initialize - failed to initialize constant buffer view for slot {}.", i);
            return false;
        }

        //~ Set Free buffer
        slot.Buffer = slot.OwnedBuffer.get();
        slot.View   = slot.OwnedView.get();
    }

    return true;
}

kfe::KFEBuffer* kfe::KFEFrameConstantBuffer::Impl::Get() const noexcept
{
    if (m_slots.empty())
    {
        return nullptr;
    }

    const std::uint16_t start = m_cursor;
    for (std::uint16_t i = 0; i < m_count; ++i)
    {
        const std::uint16_t idx = static_cast<std::uint16_t>((start + i) % m_count);
        auto& slot = m_slots[idx];

        if (!slot.Buffer) //~ Inactive slot
        {
            continue;
        }

        if (!slot.InUse) //~ If not in use, take it
        {
            m_cursor = static_cast<std::uint16_t>((idx + 1) % m_count);
            return slot.Buffer;
        }

        //~ If in use see if the fence has completed
        if (slot.Fence != nullptr)
        {
            const std::uint64_t completed = slot.Fence->GetCompletedValue();
            if (completed >= slot.FenceValue)
            {
                slot.InUse      = false;
                slot.Fence      = nullptr;
                slot.FenceValue = 0;

                m_cursor = static_cast<std::uint16_t>((idx + 1) % m_count);
                return slot.Buffer;
            }
        }
    }

    //~ Nothing free at the moment
    return nullptr;
}

kfe::KFEConstantBuffer* kfe::KFEFrameConstantBuffer::Impl::GetView(const KFEBuffer* buffer) const noexcept
{
    if (!buffer)
    {
        return nullptr;
    }

    for (const auto& slot : m_slots)
    {
        if (slot.Buffer == buffer)
        {
            return slot.View;
        }
    }

    return nullptr;
}

void kfe::KFEFrameConstantBuffer::Impl::Mark(KFEBuffer* buffer,
                                          ID3D12Fence* fence,
                                          std::uint64_t fenceValue) noexcept
{
    if (!buffer || !fence)
    {
        LOG_WARNING("called with null buffer or fence.");
        return;
    }

    for (auto& slot : m_slots)
    {
        if (slot.Buffer == buffer)
        {
            if (slot.InUse && slot.Fence != nullptr)
            {
                const std::uint64_t completed = slot.Fence->GetCompletedValue();
                if (completed < slot.FenceValue)
                {
                    LOG_ERROR(
                        "buffer marked again before previous "
                        "fence completed");
                }
            }

            slot.InUse      = true;
            slot.Fence      = fence; 
            slot.FenceValue = fenceValue;
            return;
        }
    }

    LOG_WARNING("buffer not found in attached slots.");
}

#pragma endregion
