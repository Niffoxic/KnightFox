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
#include "engine/system/interface/interface_singleton.h"

#include <memory>
#include <string>
#include <cstdint>
#include <d3d12.h>

namespace kfe
{
	/// <summary>
	/// High level shader library that:
	/// Compiles shaders on demand and,
	/// Caches compiled blobs in memory and,
	/// Persists compiled shaders to disk and,
	/// Reloads cached shaders from disk on startup / request
	/// </summary>
	class KFE_API KFEShaderLibrary final: public ISingleton<KFEShaderLibrary>
	{
		friend class ISingleton<KFEShaderLibrary>;
	public:
		struct COMPILE_DESC
		{
			std::string   SourcePath;
			std::string   EntryPoint;
			std::string   TargetProfile;
			std::string   CacheKeyOverride;
			bool          ForceRecompile  { false };
			bool          EnableDebug	  { false };
			bool          WarningsAsErrors{ true };
			std::uint32_t ReservedFlags	  { 0u };
		};

		KFEShaderLibrary(const KFEShaderLibrary&)	  = delete;
		KFEShaderLibrary(KFEShaderLibrary&&) noexcept = delete;

		KFEShaderLibrary& operator=(const KFEShaderLibrary&)	 = delete;
		KFEShaderLibrary& operator=(KFEShaderLibrary&&) noexcept = delete;

		void SetCacheDirectory(_In_ const std::string& directory);
		NODISCARD const std::string& GetCacheDirectory() const noexcept;

		NODISCARD ID3DBlob* GetShader(_In_ const std::string& path);
		NODISCARD ID3DBlob* GetShader(_In_ const COMPILE_DESC& desc);

		NODISCARD bool Contains(_In_ const std::string& cacheKey) const;
		NODISCARD bool Remove  (_In_ const std::string& cacheKey);

		void Clear() noexcept;
		NODISCARD std::size_t GetCachedShaderCount() const noexcept;

		NODISCARD bool Load(_In_ const std::string& path);
		NODISCARD bool Save(_In_ const std::string& path) const;
		NODISCARD bool ReloadFromDisk();

	private:
		 KFEShaderLibrary();
		~KFEShaderLibrary();

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
