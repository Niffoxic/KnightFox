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


kfe::rg::RenderGraphBuilder::RenderGraphBuilder(
	kfe::rg::RGResources& resources,
	kfe::rg::RenderPassDesc& desc) noexcept
	: m_pResources(&resources)
	, m_pDesc(&desc)
{}

kfe::rg::RGTextureHandle kfe::rg::RenderGraphBuilder::CreateTexture(
	const kfe::rg::RGTextureDesc& desc)
{
	if (!m_pResources)
	{
		return RGTextureHandle::Invalid();
	}

	return m_pResources->CreateTexture(desc);
}

kfe::rg::RGBufferHandle kfe::rg::RenderGraphBuilder::CreateBuffer(
	const kfe::rg::RGBufferDesc& desc)
{
	if (!m_pResources)
	{
		return RGBufferHandle::Invalid();
	}

	return m_pResources->CreateBuffer(desc);
}

void kfe::rg::RenderGraphBuilder::ReadTexture(RGTextureHandle handle)
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

void kfe::rg::RenderGraphBuilder::WriteTexture(RGTextureHandle handle)
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

void kfe::rg::RenderGraphBuilder::ReadWriteTexture(RGTextureHandle handle)
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

void kfe::rg::RenderGraphBuilder::ReadBuffer(RGBufferHandle handle)
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

void kfe::rg::RenderGraphBuilder::WriteBuffer(RGBufferHandle handle)
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
