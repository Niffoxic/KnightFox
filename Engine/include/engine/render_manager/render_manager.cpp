#include "pch.h"
#include "render_manager.h"

#include "engine/windows_manager/windows_manager.h"
#include "engine/utils/logger/logger.h"

//~ Components
#include "components/factory/factory.h"
#include "components/adapter/adapter.h"

#pragma region IMPL

class kfe::KFERenderManager::Impl
{
public:
	Impl(KFEWindows* windows);

	bool Initialize();
	bool Release   ();

	void FrameBegin(float dt);
	void FrameEnd  ();

private:
	KFEWindows* m_pWindows{ nullptr };

	//~ Components
	std::unique_ptr<KFEFactory> m_pFactory{ nullptr };
	std::unique_ptr<KFEAdapter> m_pAdapter{ nullptr };
};

#pragma endregion

kfe::KFERenderManager::KFERenderManager(KFEWindows* windows)
	: m_impl(std::make_shared<kfe::KFERenderManager::Impl>(windows))
{}

kfe::KFERenderManager::~KFERenderManager()
{
	if (m_impl) 
	{
		m_impl->Release();
	}
}

bool kfe::KFERenderManager::Initialize()
{
	return m_impl->Initialize();
}

bool kfe::KFERenderManager::Release()
{
	return m_impl->Release();
}

void kfe::KFERenderManager::OnFrameBegin(float deltaTime)
{
	m_impl->FrameBegin(deltaTime);
}

void kfe::KFERenderManager::OnFrameEnd()
{
	m_impl->FrameEnd();
}

std::string kfe::KFERenderManager::GetName() const noexcept
{
	return "RenderManager";
}

//~ Impl 
kfe::KFERenderManager::Impl::Impl(KFEWindows* windows)
	: m_pWindows(windows)
{
	m_pFactory = std::make_unique<KFEFactory>();
	m_pAdapter = std::make_unique<KFEAdapter>();
}

bool kfe::KFERenderManager::Impl::Initialize()
{
	if (!m_pFactory->Initialize())
	{
		LOG_ERROR("Failed to Initialize Factory");
		return false;
	}

	auto strategy = std::make_unique<AdapterStrategyBestVram>();
	if (!m_pAdapter->Initialize(m_pFactory.get(), strategy.get()))
	{
		LOG_ERROR("Failed to Initialize Factory");
		return false;
	}


#if defined(_DEBUG) || defined(DEBUG)
	m_pAdapter->LogAdapters();
#endif

	LOG_SUCCESS("RenderManager: All Components initialized!");
	return true;
}

bool kfe::KFERenderManager::Impl::Release()
{
	return true;
}

void kfe::KFERenderManager::Impl::FrameBegin(float dt)
{

}

void kfe::KFERenderManager::Impl::FrameEnd()
{

}
