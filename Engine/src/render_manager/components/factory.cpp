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
#include "engine/render_manager/components/factory.h"

#include <vector>
#include <memory>
#include <wrl.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include "engine/system/exception/dx_exception.h"
#include "engine/utils/logger.h"

class kfe::KFEFactory::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	[[nodiscard]] bool			 Initialize		   ();
	[[nodiscard]] IDXGIFactory7* GetNative		   () const noexcept;
	[[nodiscard]] bool			 IsTearingSupported() const noexcept;

private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> m_pFactory{ nullptr };
	bool m_bTearingSupported{ false };
};

kfe::KFEFactory::KFEFactory()
	: m_impl(std::make_unique<kfe::KFEFactory::Impl>())
{}

kfe::KFEFactory::~KFEFactory() = default;

bool kfe::KFEFactory::Initialize()
{
	return m_impl->Initialize();
}

IDXGIFactory7* kfe::KFEFactory::GetNative() const noexcept
{
	return m_impl->GetNative();
}

bool kfe::KFEFactory::IsTearingSupported() const noexcept
{
	return m_impl->IsTearingSupported();
}

//~ IMPL
bool kfe::KFEFactory::Impl::Initialize()
{
#if defined(DEBUG) || defined(_DEBUG)
	UINT flags = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT flags = 0u;
#endif

	THROW_DX_IF_FAILS(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_pFactory)));
	LOG_INFO("Created DXGI Factory.");

	BOOL tearing = FALSE;
	const HRESULT hr = m_pFactory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&tearing, sizeof(tearing)
	);

	m_bTearingSupported = SUCCEEDED(hr) && tearing;

	LOG_INFO("Tearing Support: {}", m_bTearingSupported);

	return true;
}

IDXGIFactory7* kfe::KFEFactory::Impl::GetNative() const noexcept
{
	return m_pFactory.Get();
}

bool kfe::KFEFactory::Impl::IsTearingSupported() const noexcept
{
	return m_bTearingSupported;
}
