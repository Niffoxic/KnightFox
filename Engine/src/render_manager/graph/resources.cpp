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
#include "engine/render_manager/graph/resources.h"

#include <vector>
#include <assert.h>

#pragma region Impl_declaration

class kfe::rg::RGResources::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	NODISCARD RGTextureHandle CreateTexture(const RGTextureDesc& desc);
	NODISCARD RGBufferHandle  CreateBuffer (const RGBufferDesc& desc);

	NODISCARD const RGTextureDesc& GetTextureDesc(RGTextureHandle handle) const;
	NODISCARD const RGBufferDesc& GetBufferDesc  (RGBufferHandle handle) const;

	void Reset() noexcept;

	NODISCARD std::uint32_t GetTextureCount() const noexcept;
	NODISCARD std::uint32_t GetBufferCount () const noexcept;

private:
	NODISCARD RGTextureHandle GetNextTextureHandle() const noexcept;
	NODISCARD RGBufferHandle  GetNextBufferHandle () const noexcept;

private:
	std::vector<RGTextureDesc> m_textures;
	std::vector<RGBufferDesc>  m_buffers;
};

#pragma endregion

#pragma region RGResource_Implementation

kfe::rg::RGResources::RGResources()
	: m_impl(std::make_unique<kfe::rg::RGResources::Impl>())
{}

kfe::rg::RGResources::~RGResources() = default;

kfe::rg::RGResources::RGResources					 (RGResources&&) noexcept = default;
kfe::rg::RGResources& kfe::rg::RGResources::operator=(RGResources&&) noexcept = default;

_Use_decl_annotations_
kfe::rg::RGTextureHandle kfe::rg::RGResources::CreateTexture(const RGTextureDesc& desc)
{
	return m_impl->CreateTexture(desc);
}

_Use_decl_annotations_
kfe::rg::RGBufferHandle kfe::rg::RGResources::CreateBuffer(const RGBufferDesc& desc)
{
	return m_impl->CreateBuffer(desc);
}

_Use_decl_annotations_
const kfe::rg::RGTextureDesc& kfe::rg::RGResources::GetTextureDesc(RGTextureHandle handle) const
{
	return m_impl->GetTextureDesc(handle);
}

_Use_decl_annotations_
const kfe::rg::RGBufferDesc& kfe::rg::RGResources::GetBufferDesc(RGBufferHandle handle) const
{
	return m_impl->GetBufferDesc(handle);
}

void kfe::rg::RGResources::Reset() noexcept
{
	m_impl->Reset();
}

_Use_decl_annotations_
std::uint32_t kfe::rg::RGResources::GetTextureCount() const noexcept
{
	return m_impl->GetTextureCount();
}

_Use_decl_annotations_
std::uint32_t kfe::rg::RGResources::GetBufferCount() const noexcept
{
	return m_impl->GetBufferCount();
}

#pragma endregion

#pragma region Impl_Implementation

_Use_decl_annotations_
kfe::rg::RGTextureHandle kfe::rg::RGResources::Impl::CreateTexture(const RGTextureDesc& desc)
{
	if (!desc.IsValid())
	{
		return RGTextureHandle::Invalid();
	}

	auto index = GetNextTextureHandle();
	m_textures.emplace_back(desc);
	return index;
}

_Use_decl_annotations_
kfe::rg::RGBufferHandle kfe::rg::RGResources::Impl::CreateBuffer(const RGBufferDesc& desc)
{
	if (!desc.IsValid())
	{
		return RGBufferHandle::Invalid();
	}

	auto index = GetNextBufferHandle();
	m_buffers.emplace_back(desc);
	return index;
}

_Use_decl_annotations_
const kfe::rg::RGTextureDesc& kfe::rg::RGResources::Impl::GetTextureDesc(RGTextureHandle handle) const
{
	assert(handle.IsValid());
	assert(handle.Index < m_textures.size());
	
	return m_textures[handle.Index];
}

_Use_decl_annotations_
const kfe::rg::RGBufferDesc& kfe::rg::RGResources::Impl::GetBufferDesc(RGBufferHandle handle) const
{
	assert(handle.IsValid());
	assert(handle.Index < m_textures.size());

	return m_buffers[handle.Index];
}

void kfe::rg::RGResources::Impl::Reset() noexcept
{
	m_buffers .clear();
	m_textures.clear();
}

_Use_decl_annotations_
std::uint32_t kfe::rg::RGResources::Impl::GetTextureCount() const noexcept
{
	return m_textures.size();
}

_Use_decl_annotations_
std::uint32_t kfe::rg::RGResources::Impl::GetBufferCount() const noexcept
{
	return m_buffers.size();
}

_Use_decl_annotations_
kfe::rg::RGTextureHandle kfe::rg::RGResources::Impl::GetNextTextureHandle() const noexcept
{
	std::uint32_t index = static_cast<std::uint32_t>(m_textures.size());
	return { index };
}

_Use_decl_annotations_
kfe::rg::RGBufferHandle kfe::rg::RGResources::Impl::GetNextBufferHandle() const noexcept
{
	std::uint32_t index = static_cast<std::uint32_t>(m_buffers.size());
	return { index };
}

#pragma endregion
