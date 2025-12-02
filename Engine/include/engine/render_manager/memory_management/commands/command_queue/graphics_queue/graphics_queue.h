#pragma once

#include "EngineAPI.h"

#include <memory>

class ID3D12CommandQueue;

namespace kfe 
{
	class KFEDevice;

	class KFE_API KFEGraphicsCmdQ
	{
	public:
		 KFEGraphicsCmdQ();
		~KFEGraphicsCmdQ();

		[[nodiscard]] bool Initialize(const KFEDevice* device);

		[[nodiscard]] bool				  Release	   ()		noexcept;
		[[nodiscard]] bool				  IsInitialized() const noexcept;
		[[nodiscard]] ID3D12CommandQueue* GetNative    () const noexcept;

	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
	};
} // namespace kfe
