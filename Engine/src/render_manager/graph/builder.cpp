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
#include "engine/render_manager/graph/builder.h"
#include "engine/render_manager/graph/resources.h"


kfe::rg::RGBuilder::RGBuilder(
	kfe::rg::RGResources& resources,
	kfe::rg::RenderPassDesc& desc) noexcept
	: m_pResources(&resources)
	, m_pDesc(&desc)
{}

_Use_decl_annotations_
kfe::rg::RGTextureHandle kfe::rg::RGBuilder::CreateTexture(
	const kfe::rg::RGTextureDesc& desc)
{
	if (!m_pResources)
	{
		return RGTextureHandle::Invalid();
	}

	return m_pResources->CreateTexture(desc);
}

_Use_decl_annotations_
kfe::rg::RGBufferHandle kfe::rg::RGBuilder::CreateBuffer(
	const kfe::rg::RGBufferDesc& desc)
{
	if (!m_pResources)
	{
		return RGBufferHandle::Invalid();
	}

	return m_pResources->CreateBuffer(desc);
}

void kfe::rg::RGBuilder::ReadTexture(RGTextureHandle handle)
{
	if (!m_pDesc || !handle.IsValid())
	{
		return;
	}

	RGTextureAccess access{};
	access.Handle = handle;
	access.Access = RGResourceAccess::Read;

	m_pDesc->TextureInputs.push_back(access);
}

void kfe::rg::RGBuilder::WriteTexture(RGTextureHandle handle)
{
	if (!m_pDesc || !handle.IsValid())
	{
		return;
	}

	RGTextureAccess access{};
	access.Handle = handle;
	access.Access = RGResourceAccess::Write;

	m_pDesc->TextureOutputs.push_back(access);
}

void kfe::rg::RGBuilder::ReadWriteTexture(RGTextureHandle handle)
{
	if (!m_pDesc || !handle.IsValid())
	{
		return;
	}

	RGTextureAccess access{};
	access.Handle = handle;
	access.Access = RGResourceAccess::ReadWrite;

	m_pDesc->TextureInputs .push_back(access);
	m_pDesc->TextureOutputs.push_back(access);
}

void kfe::rg::RGBuilder::ReadBuffer(RGBufferHandle handle)
{
	if (!m_pDesc || !handle.IsValid())
	{
		return;
	}

	RGBufferAccess access{};
	access.Handle = handle;
	access.Access = RGResourceAccess::Read;

	m_pDesc->BufferInputs.push_back(access);
}

void kfe::rg::RGBuilder::WriteBuffer(RGBufferHandle handle)
{
	if (!m_pDesc || !handle.IsValid())
	{
		return;
	}

	RGBufferAccess access{};
	access.Handle = handle;
	access.Access = RGResourceAccess::Write;

	m_pDesc->BufferOutputs.push_back(access);
}
