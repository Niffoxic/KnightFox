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
#include "engine/render_manager/api/buffer/vertex_buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/utils/logger.h"

#include <d3d12.h>

#pragma region Impl_Declaration

class kfe::KFEVertexBuffer::Impl
{
public:
	 Impl() = default;
	~Impl() 
	{
		if (!Destroy())
		{
			LOG_ERROR("Failed to Destroy Resorce Succesfully! Might Leak Resource on the GPU!");
		}
	}

	NODISCARD bool Initialize(_In_ const KFE_VERTEX_BUFFER_CREATE_DESC& desc);

	NODISCARD bool Destroy		()		 noexcept;
	NODISCARD bool IsInitialized() const noexcept;

	NODISCARD _Ret_maybenull_       KFEBuffer* GetBuffer()       noexcept;
	NODISCARD _Ret_maybenull_ const KFEBuffer* GetBuffer() const noexcept;

	NODISCARD D3D12_VERTEX_BUFFER_VIEW GetView() const noexcept;

	NODISCARD std::uint32_t GetStrideInBytes() const noexcept;
	NODISCARD std::uint64_t GetOffsetInBytes() const noexcept;
	NODISCARD std::uint32_t GetVertexCount  () const noexcept;

private:
	// Cached D3D12 view
	D3D12_VERTEX_BUFFER_VIEW m_vertexView{};
	KFEBuffer*				 m_pResourceBuffer{ nullptr };
	KFEDevice*				 m_pDevice		  { nullptr };
	std::uint32_t			 m_strideInBytes  { 0u };
	std::uint64_t			 m_offsetInBytes  { 0u };
	std::uint32_t			 m_vertexCount	  { 0u };

	bool		m_bInitialized{ false };
};

#pragma endregion

#pragma region VertexBuffer_Implemention


kfe::KFEVertexBuffer::KFEVertexBuffer() noexcept
	: m_impl(std::make_unique<kfe::KFEVertexBuffer::Impl>())
{}

kfe::KFEVertexBuffer::~KFEVertexBuffer() noexcept = default;
kfe::KFEVertexBuffer::KFEVertexBuffer				 (KFEVertexBuffer&& other) noexcept = default;
kfe::KFEVertexBuffer& kfe::KFEVertexBuffer::operator=(KFEVertexBuffer&& other) noexcept = default;

_Use_decl_annotations_
std::string kfe::KFEVertexBuffer::GetName() const noexcept
{
	return "KFEVertexBuffer";
}

_Use_decl_annotations_
std::string kfe::KFEVertexBuffer::GetDescription() const noexcept
{
	return "Vertex Buffer View: Wraps ID3D12Resource";
}

_Use_decl_annotations_
bool kfe::KFEVertexBuffer::Initialize(const KFE_VERTEX_BUFFER_CREATE_DESC& desc)
{
	return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFEVertexBuffer::Destroy() noexcept
{
	return m_impl->Destroy();
}

_Use_decl_annotations_
bool kfe::KFEVertexBuffer::IsInitialized() const noexcept
{
	return m_impl->IsInitialized();
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEVertexBuffer::GetBuffer() noexcept
{
	return m_impl->GetBuffer();
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEVertexBuffer::GetBuffer() const noexcept
{
	return m_impl->GetBuffer();
}

_Use_decl_annotations_
D3D12_VERTEX_BUFFER_VIEW kfe::KFEVertexBuffer::GetView() const noexcept
{
	return m_impl->GetView();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEVertexBuffer::GetStrideInBytes() const noexcept
{
	return m_impl->GetStrideInBytes();
}

_Use_decl_annotations_
std::uint64_t kfe::KFEVertexBuffer::GetOffsetInBytes() const noexcept
{
	return m_impl->GetOffsetInBytes();
}

_Use_decl_annotations_
std::uint32_t kfe::KFEVertexBuffer::GetVertexCount() const noexcept
{
	return m_impl->GetVertexCount();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
bool kfe::KFEVertexBuffer::Impl::Initialize(const KFE_VERTEX_BUFFER_CREATE_DESC& desc)
{
    // Validate device
    if (!desc.Device || !desc.Device->GetNative())
    {
        LOG_ERROR("KFEVertexBuffer::Impl::Initialize: Device or native device is null.");
        return false;
    }

    // Validate underlying buffer
    if (!desc.ResourceBuffer || !desc.ResourceBuffer->GetNative())
    {
        LOG_ERROR("KFEVertexBuffer::Impl::Initialize: ResourceBuffer or its native resource is null.");
        return false;
    }

    if (desc.StrideInBytes == 0u)
    {
        LOG_ERROR("KFEVertexBuffer::Impl::Initialize: StrideInBytes must be > 0.");
        return false;
    }

    // If already initialized, destroy and re-init
    if (IsInitialized())
    {
        LOG_WARNING("KFEVertexBuffer::Impl::Initialize: Already initialized. Destroying and recreating.");
        if (!Destroy())
        {
            LOG_ERROR("KFEVertexBuffer::Impl::Initialize: Destroy failed; cannot reinitialize.");
            return false;
        }
    }

    m_pDevice           = desc.Device;
    m_pResourceBuffer   = desc.ResourceBuffer;
    m_offsetInBytes     = desc.OffsetInBytes;

    // Get resource info
    ID3D12Resource* resource   = m_pResourceBuffer->GetNative     ();
    const auto      gpuVA      = resource->GetGPUVirtualAddress   ();
    const auto      bufferSize = m_pResourceBuffer->GetSizeInBytes();

    if (m_offsetInBytes >= bufferSize)
    {
        LOG_ERROR(
            "KFEVertexBuffer::Impl::Initialize: OffsetInBytes ({}) is >= buffer size ({}).",
            m_offsetInBytes, bufferSize);
        return false;
    }

    const std::uint64_t usableSize = bufferSize - m_offsetInBytes;
    if (usableSize < desc.StrideInBytes)
    {
        LOG_ERROR(
            "KFEVertexBuffer::Impl::Initialize: Usable size ({}) is smaller than stride ({}).",
            usableSize, desc.StrideInBytes);
        return false;
    }

    m_vertexView.BufferLocation = gpuVA + m_offsetInBytes;
    m_vertexView.SizeInBytes    = static_cast<UINT>(usableSize);
    m_vertexView.StrideInBytes  = desc.StrideInBytes;

    m_bInitialized = true;
    return true;
}

_Use_decl_annotations_
bool kfe::KFEVertexBuffer::Impl::Destroy() noexcept
{
    m_vertexView.BufferLocation = 0u;
    m_vertexView.SizeInBytes    = 0u;
    m_vertexView.StrideInBytes  = 0u;

    m_pResourceBuffer   = nullptr;
    m_pDevice           = nullptr;
    m_offsetInBytes     = 0u;
    m_bInitialized      = false;

    return true;
}

_Use_decl_annotations_
bool kfe::KFEVertexBuffer::Impl::IsInitialized() const noexcept
{
    return m_bInitialized;
}

_Use_decl_annotations_
kfe::KFEBuffer* kfe::KFEVertexBuffer::Impl::GetBuffer() noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
const kfe::KFEBuffer* kfe::KFEVertexBuffer::Impl::GetBuffer() const noexcept
{
    return m_pResourceBuffer;
}

_Use_decl_annotations_
D3D12_VERTEX_BUFFER_VIEW kfe::KFEVertexBuffer::Impl::GetView() const noexcept
{
    return m_vertexView;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEVertexBuffer::Impl::GetStrideInBytes() const noexcept
{
    return m_vertexView.StrideInBytes;
}

_Use_decl_annotations_
std::uint64_t kfe::KFEVertexBuffer::Impl::GetOffsetInBytes() const noexcept
{
    return m_offsetInBytes;
}

_Use_decl_annotations_
std::uint32_t kfe::KFEVertexBuffer::Impl::GetVertexCount() const noexcept
{
    if (m_vertexView.StrideInBytes == 0u)
    {
        return 0u;
    }

    const std::uint64_t count =
        static_cast<std::uint64_t>(m_vertexView.SizeInBytes) /
        static_cast<std::uint64_t>(m_vertexView.StrideInBytes);

    return static_cast<std::uint32_t>(count);
}

#pragma endregion
