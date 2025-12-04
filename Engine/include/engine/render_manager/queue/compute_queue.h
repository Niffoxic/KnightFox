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

#include <cstdint>
#include <memory>

#include "engine/system/common_types.h"
#include "engine/core.h"

class ID3D12CommandQueue;

namespace kfe
{
	class KFEDevice;

	class KFE_API KFEComputeCmdQ
	{
	public:
		 KFEComputeCmdQ();
		~KFEComputeCmdQ();

		NODISCARD bool Initialize(const KFEDevice* device);

		NODISCARD bool				  Release	   ()		noexcept;
		NODISCARD bool				  IsInitialized() const noexcept;
		NODISCARD ID3D12CommandQueue* GetNative	   () const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
