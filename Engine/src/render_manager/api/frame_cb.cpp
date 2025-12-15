#include "pch.h"
#include "engine/render_manager/api/frame_cb.h"

namespace kfe
{
    bool KFEFrameConstantBuffer::Initialize(_In_ const KFE_FRAME_CONSTANT_BUFFER_DESC& desc) noexcept
    {
        if (m_initialized)
            return true;

        if (!desc.Device || !desc.ResourceHeap)
            return false;

        if (desc.FrameCount == 0u)
            return false;

        if (desc.SizeInBytes == 0u)
            return false;

        m_frameCount = desc.FrameCount;
        m_frameIndex = 0u;
        m_sizeBytes = kfe_helpers::AlignTo256(desc.SizeInBytes);

        m_slices.clear();
        m_slices.resize(static_cast<std::size_t>(m_frameCount));

        if (!CreateSlices(desc))
        {
            Destroy();
            return false;
        }

        m_initialized = true;
        return true;
    }

    bool KFEFrameConstantBuffer::Destroy() noexcept
    {
        for (auto& s : m_slices)
        {
            s.View.reset();
            s.Buffer.reset();
        }

        m_slices.clear();
        m_frameCount = 0u;
        m_frameIndex = 0u;
        m_sizeBytes = 0u;
        m_initialized = false;

        return true;
    }

    void KFEFrameConstantBuffer::Step() noexcept
    {
        if (!m_initialized || m_frameCount == 0u)
            return;

        m_frameIndex = (m_frameIndex + 1u) % m_frameCount;
    }

    bool KFEFrameConstantBuffer::IsInitialized() const noexcept
    {
        return m_initialized;
    }

    std::uint32_t KFEFrameConstantBuffer::GetFrameCount() const noexcept
    {
        return m_frameCount;
    }

    std::uint32_t KFEFrameConstantBuffer::GetFrameIndex() const noexcept
    {
        return m_frameIndex;
    }

    std::uint32_t KFEFrameConstantBuffer::GetSizeInBytes() const noexcept
    {
        return m_sizeBytes;
    }

    KFEBuffer* KFEFrameConstantBuffer::GetBuffer() const noexcept
    {
        if (!m_initialized || m_slices.empty())
            return nullptr;

        return m_slices[static_cast<std::size_t>(m_frameIndex)].Buffer.get();
    }

    KFEConstantBuffer* KFEFrameConstantBuffer::GetView() const noexcept
    {
        if (!m_initialized || m_slices.empty())
            return nullptr;

        return m_slices[static_cast<std::size_t>(m_frameIndex)].View.get();
    }

    void* KFEFrameConstantBuffer::GetMappedData() const noexcept
    {
        KFEConstantBuffer* v = GetView();
        if (!v) return nullptr;
        return v->GetMappedData();
    }

    bool KFEFrameConstantBuffer::CreateSlices(_In_ const KFE_FRAME_CONSTANT_BUFFER_DESC& desc) noexcept
    {
        for (std::uint32_t i = 0u; i < m_frameCount; ++i)
        {
            auto& slice = m_slices[static_cast<std::size_t>(i)];

            slice.Buffer = std::make_unique<KFEBuffer>();

            KFE_CREATE_BUFFER_DESC buf{};
            buf.Device = desc.Device;
            buf.HeapType = D3D12_HEAP_TYPE_UPLOAD;
            buf.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
            buf.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
            buf.SizeInBytes = m_sizeBytes;

            if (!slice.Buffer->Initialize(buf))
                return false;

            slice.View = std::make_unique<KFEConstantBuffer>();

            KFE_CONSTANT_BUFFER_CREATE_DESC view{};
            view.Device = desc.Device;
            view.OffsetInBytes = 0u;
            view.ResourceBuffer = slice.Buffer.get();
            view.ResourceHeap = desc.ResourceHeap;
            view.SizeInBytes = m_sizeBytes;

            if (!slice.View->Initialize(view))
                return false;
        }

        return true;
    }

} // namespace kfe
