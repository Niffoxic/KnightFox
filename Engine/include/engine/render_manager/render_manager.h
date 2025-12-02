// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

/*
 *  -----------------------------------------------------------------------------
 *  Project   : PixelFox (WMG Warwick - Module 1)
 *  Author    : Niffoxic (a.k.a Harsh Dubey)
 *  License   : MIT
 *  -----------------------------------------------------------------------------
 */

#pragma once
#include "EngineAPI.h"

#include "engine/core/interface/interface_manager.h"
#include <memory>

namespace kfe
{
	class KFEWindows;

	class KFE_API KFERenderManager final : public IManager
	{
	public:
		 KFERenderManager(KFEWindows* windows);
		~KFERenderManager();

		//~ Inherited via IManager
		bool		Initialize	()				  override;
		bool		Release		()				  override;
		void		OnFrameBegin(float deltaTime) override;
		void		OnFrameEnd	()				  override;
		std::string GetName		() const noexcept override;

	private:
		class Impl;
		std::shared_ptr<Impl>		GetImpl		()		 { return m_impl; }
		std::shared_ptr<const Impl> GetConstImpl() const { return m_impl; }

		std::shared_ptr<Impl> m_impl{ nullptr };
	};
} // namespace kfe
