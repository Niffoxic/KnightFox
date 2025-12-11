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
#include <sal.h>
#include <memory>

#include "system/common_types.h"
#include "engine/map/world.h"

namespace kfe
{	typedef struct _KFE_ENGINE_CREATE_DESC
	{
		_In_ KFE_WINDOW_CREATE_DESC WindowsDesc;
	} KFE_ENGINE_CREATE_DESC;

	class KFE_API IKFEngine
	{
	public:
		IKFEngine(_In_ const KFE_ENGINE_CREATE_DESC& desc);
		virtual ~IKFEngine();

		IKFEngine(_In_ const IKFEngine&) = delete;
		IKFEngine(_Inout_ IKFEngine&&)   = delete;

		IKFEngine& operator=(_In_ const IKFEngine&) = delete;
		IKFEngine& operator=(_Inout_ IKFEngine&&)	= delete;

		_NODISCARD _Check_return_ _Success_(return != false)
		bool Init();

		_Success_(return == S_OK)
		int Execute();

		KFEWorld* GetWorld() const;

	protected:
		//~ Application Must Implement them
		_NODISCARD _Check_return_
		virtual bool InitApplication() = 0;
		virtual void BeginPlay		() = 0;
		virtual void Release		() = 0;

		virtual void Tick(_In_ float deltaTime) = 0;

	private:
		class Impl;
		std::shared_ptr<Impl>		GetImpl		()		 { return m_impl; }
		std::shared_ptr<const Impl> GetConstImpl() const { return m_impl; }
		std::shared_ptr<Impl>		m_impl{ nullptr };
	};
} // namespace kfe
