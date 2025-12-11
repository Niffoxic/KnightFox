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
#include "engine/editor/editor.h"
#include "engine/editor/widgets/assets_panel.h"
#include "engine/editor/widgets/creational_panel.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "engine/map/world.h"
#include <utility>

#pragma region Impl_Definition

class kfe::KFEEditor::Impl
{
public:
	Impl(KFEWorld* world)
		: m_pWorld(world)
	{
	}
	~Impl() = default;

	NODISCARD bool Initialize();
	NODISCARD bool Release();

	void Update(float deltaTime);
	void Render();
private:
	void SetupDockspace();

private:
	KFEAssetPanel    m_assetsPanel{};
	KFECreationPanel m_creationPanel{};
	KFEWorld* m_pWorld{ nullptr };

	ImGuiID m_mainDockspaceId{ 0 };
	bool    m_dockLayoutInitialized{ false };
};

#pragma endregion

#pragma region Editor_Body

kfe::KFEEditor::KFEEditor(KFEWorld* world) noexcept
	: m_impl(std::make_unique<Impl>(world))
{
}

kfe::KFEEditor::~KFEEditor() noexcept = default;

kfe::KFEEditor::KFEEditor(KFEEditor&& other) noexcept
	: m_impl(std::move(other.m_impl))
{
}

kfe::KFEEditor& kfe::KFEEditor::operator=(KFEEditor&& other) noexcept
{
	if (this != &other)
	{
		m_impl = std::move(other.m_impl);
	}
	return *this;
}

_Use_decl_annotations_
bool kfe::KFEEditor::Initialize()
{
	return m_impl->Initialize();
}

_Use_decl_annotations_
bool kfe::KFEEditor::Release()
{
	if (!m_impl)
	{
		return true;
	}

	const bool result = m_impl->Release();
	m_impl.reset();
	return result;
}

_Use_decl_annotations_
void kfe::KFEEditor::OnFrameBegin(float deltaTime)
{
	if (m_impl)
	{
		m_impl->Update(deltaTime);
	}
}

void kfe::KFEEditor::OnFrameEnd()
{
	if (m_impl)
	{
		m_impl->Render();
	}
}

_Use_decl_annotations_
std::string kfe::KFEEditor::GetName() const noexcept
{
	return "KFEEditor";
}

#pragma endregion

#pragma region Impl_Body

_Use_decl_annotations_
bool kfe::KFEEditor::Impl::Initialize()
{
	// Assets panel
	m_assetsPanel.SetPanelName("Assets");
	m_assetsPanel.SetRootPath("assets");
	m_assetsPanel.SetVisible(true);

	bool ok = m_assetsPanel.Initialize();

	// Creation panel on the left; uses only KFEWorld
	m_creationPanel.SetPanelName("Create");
	m_creationPanel.SetVisible(true);
	ok = m_creationPanel.Initialize(m_pWorld) && ok;

	return ok;
}

_Use_decl_annotations_
bool kfe::KFEEditor::Impl::Release()
{
	bool ok = true;
	ok = m_creationPanel.Release() && ok;
	ok = m_assetsPanel.Release() && ok;
	return ok;
}

void kfe::KFEEditor::Impl::Update(float /*dt*/)
{
	// Editor-level per-frame logic can go here later if needed
}

void kfe::KFEEditor::Impl::Render()
{
	SetupDockspace();

	m_assetsPanel.Draw();
	m_creationPanel.Draw();

	ImGui::End();
}

void kfe::KFEEditor::Impl::SetupDockspace()
{
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	windowFlags |= ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoBackground;

	ImGui::Begin("KFE_EditorDockHost", nullptr, windowFlags);
	ImGui::PopStyleVar(2);

	m_mainDockspaceId = ImGui::GetID("KFE_EditorDockspace");
	ImGui::DockSpace(
		m_mainDockspaceId,
		ImVec2(0.0f, 0.0f),
		ImGuiDockNodeFlags_PassthruCentralNode
	);

}

#pragma endregion
