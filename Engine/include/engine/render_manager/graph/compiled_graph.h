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

#include "types.h"

#include "pass.h"

#include <vector>
#include <cstdint>

#include <wrl/client.h>
#include <d3d12.h>

namespace kfe
{
	struct FrameContext;
	class KFEBuffer;
	class KFETexture;
}

namespace kfe::rg
{
	struct CompiledPass
	{
		RenderPass* Pass{ nullptr };

		std::vector<D3D12_RESOURCE_BARRIER> BarriersBefore;

		NODISCARD bool IsValid() const noexcept
		{
			return Pass != nullptr && Pass->IsValid();
		}

		void Reset() noexcept
		{
			Pass = nullptr;
			BarriersBefore.clear();
		}
	};

	class RGCompiled
	{
	public:
		 RGCompiled();
		~RGCompiled();

		RGCompiled			 (const RGCompiled&) = delete;
		RGCompiled& operator=(const RGCompiled&) = delete;

		RGCompiled			 (RGCompiled&&) noexcept;
		RGCompiled& operator=(RGCompiled&&) noexcept;

		void Execute(const FrameContext& frameCtx);

	private:
		std::vector<CompiledPass> m_ExecutionOrder;
		std::vector<KFETexture*>  m_TextureResources;
		std::vector<KFEBuffer*>   m_BufferResources;

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVs;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_DSVs;
		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_SRVs;
		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_UAVs;

		friend class RenderGraph;
	};

} // namespace kfe::rg
