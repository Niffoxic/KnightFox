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
#include "interface_scene.h"

#include <memory>

namespace kfe 
{
	class KFE_API KEFCubeSceneObject final: public IKFESceneObject
	{
	public:
		 KEFCubeSceneObject();
		 KEFCubeSceneObject(const std::uint32_t multiple);
		~KEFCubeSceneObject() override;

		// Inherited via IKFESceneObject
		std::string GetName		  () const noexcept override;
		std::string GetDescription() const noexcept override;
		
		void Update(const KFE_UPDATE_OBJECT_DESC& desc)     override;
		bool Build (_In_ const KFE_BUILD_OBJECT_DESC& desc) override;
		
		bool Destroy() override;
		
		void Render(_In_ const KFE_RENDER_OBJECT_DESC& desc) override;
	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
