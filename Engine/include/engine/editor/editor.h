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
#include "engine/system/interface/interface_manager.h"

#include <memory>
#include <string>

namespace kfe
{
	class KFE_API KFEEditor final : public IManager
	{
	public:
		 KFEEditor() noexcept;
		~KFEEditor() noexcept override;

		KFEEditor			(const KFEEditor&) = delete;
		KFEEditor& operator=(const KFEEditor&) = delete;

		KFEEditor			(KFEEditor&&) noexcept;
		KFEEditor& operator=(KFEEditor&&) noexcept;

		// IManager interface
		NODISCARD bool Initialize() override;
		NODISCARD bool Release	 () override;

		void OnFrameBegin(_In_ float deltaTime) override;
		void OnFrameEnd  ()                     override;

		NODISCARD std::string GetName() const noexcept override;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
