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

#include <cstdint>
#include <memory>
#include <string>

enum	DXGI_FORMAT;
enum	D3D12_RTV_DIMENSION;
struct	D3D12_CPU_DESCRIPTOR_HANDLE;

namespace kfe 
{
	class KFEDevice;
	class KFERTVHeap;
	class KFETexture;

	typedef struct _KFE_RTV_CREATE_DESC
	{
		_In_ KFEDevice*  Device;
		_In_ KFERTVHeap* Heap;
		_In_ KFETexture* Texture;

		DXGI_FORMAT			Format;
		D3D12_RTV_DIMENSION ViewDimension;

		std::uint32_t MipSlice;
		std::uint32_t FirstArraySlice;
		std::uint32_t ArraySize;
		std::uint32_t PlaneSlice;

		// If set to INVALID the RTV will allocate a new slot
		std::uint32_t DescriptorIndex;

	} KFE_RTV_CREATE_DESC;

	class KFE_API KFERenderTargetView final: public IKFEObject
	{
	public:
		 KFERenderTargetView() noexcept;
		~KFERenderTargetView() noexcept;

		KFERenderTargetView(const KFERenderTargetView&) = delete;
		KFERenderTargetView(KFERenderTargetView&&) noexcept;

		KFERenderTargetView& operator=(const KFERenderTargetView&) = delete;
		KFERenderTargetView& operator=(KFERenderTargetView&&) noexcept;

		NODISCARD bool Initialize(const KFE_RTV_CREATE_DESC& desc);
		
		NODISCARD bool Destroy	   ()		noexcept;
		NODISCARD bool IsInitialize() const noexcept;

		NODISCARD KFERTVHeap* GetAttachedHeap() const noexcept;
		NODISCARD KFETexture* GetTexture	 () const noexcept;

		NODISCARD std::uint32_t				  GetDescriptorIndex() const noexcept;
		NODISCARD bool						  HasValidDescriptor() const noexcept;
		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle		() const noexcept;

		// View parameters
		NODISCARD DXGI_FORMAT         GetFormat		  () const noexcept;
		NODISCARD D3D12_RTV_DIMENSION GetViewDimension() const noexcept;

		NODISCARD std::uint32_t GetMipSlice		  () const noexcept;
		NODISCARD std::uint32_t GetFirstArraySlice() const noexcept;
		NODISCARD std::uint32_t GetArraySize	  () const noexcept;
		NODISCARD std::uint32_t GetPlaneSlice	  () const noexcept;

		// Inherited via IKFEObject
		std::string GetName		  () const noexcept override;
		std::string GetDescription() const noexcept override;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
