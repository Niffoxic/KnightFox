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
#include "engine/render_manager/graph/types.h"
#include <d3d12.h>

constexpr kfe::rg::ResourceState::ResourceState() noexcept
	: CurrentState(D3D12_RESOURCE_STATE_COMMON)
{
}

constexpr kfe::rg::ResourceState::ResourceState(D3D12_RESOURCE_STATES initial) noexcept
	: CurrentState(initial)
{
}

constexpr D3D12_RESOURCE_STATES kfe::rg::ResourceState::Get() const noexcept
{
	return CurrentState;
}

constexpr void kfe::rg::ResourceState::Set(D3D12_RESOURCE_STATES newState) noexcept
{
	CurrentState = newState;
}
