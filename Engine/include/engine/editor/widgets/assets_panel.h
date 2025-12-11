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

#include <string>
#include <functional>
#include <cstdint>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include "imgui/imgui.h"

namespace kfe
{
    class KFE_API KFEAssetPanel final
    {
    public:
        using AssetSelectedCallback = std::function<void(const std::string&)>;

        static constexpr const char* kPayloadType = "KFE_ASSET_V1";

        enum class Kind : std::uint16_t
        {
            File = 0,
            Directory = 1
        };

#pragma pack(push,1)
        struct PayloadHeader
        {
            std::uint32_t magic{ 0xA551'7BAD };
            std::uint16_t version{ 1 };
            std::uint16_t kind{ 0 };
        };
#pragma pack(pop)

    public:
        KFEAssetPanel() noexcept;
        explicit KFEAssetPanel(_In_ const std::string& panelName) noexcept;
        ~KFEAssetPanel() noexcept;

        KFEAssetPanel(const KFEAssetPanel&) = delete;
        KFEAssetPanel& operator=(const KFEAssetPanel&) = delete;

        KFEAssetPanel(KFEAssetPanel&&) noexcept;
        KFEAssetPanel& operator=(KFEAssetPanel&&) noexcept;

        NODISCARD bool Initialize();
        NODISCARD bool Release();

        static bool ParsePayload(
            _In_ const ImGuiPayload* payload,
            _Out_ PayloadHeader&     outHeader,
            _Out_ std::string&       outPathUtf8);

        void Draw();
        void SetDockspaceId(std::uint32_t dockspaceId) noexcept;
        NODISCARD std::uint32_t GetDockspaceId() const noexcept;

        void SetVisible(bool visible) noexcept;
        NODISCARD bool IsVisible() const noexcept;

        void SetRootPath(_In_ const std::string& path);
        NODISCARD const std::string& GetRootPath() const noexcept;

        void SetOnAssetSelected(_In_ AssetSelectedCallback callback);


        NODISCARD const std::string& GetPanelName() const noexcept;
        void SetPanelName(_In_ const std::string& name);

        void SetIcons(ImTextureID folder, ImTextureID file) noexcept;
        void SetExtIcon(std::string extNoDotLower, ImTextureID icon);

    private:
        struct Entry
        {
            std::filesystem::path Path;
            std::string           Name;
            std::string           Ext;
            bool                  IsDir{ false };
            std::uint64_t         Size{ 0 };
            ImTextureID           Icon{ 0ll };
        };

    private:
        //~ High level UI pieces
        void Toolbar();
        void Breadcrumbs();
        void DrawGrid();
        void DrawList();

        //~ Navigation / scanning
        void Rescan();
        void GoUp();
        void GoBack();
        void GoForward();
        void Enter(const std::filesystem::path& child);
        void ScanDir(const std::filesystem::path& p);
        void RebuildItemsForSearch();

        //~ Helpers
        ImTextureID IconFor(const Entry& e) const;
        static bool IsHidden(const std::filesystem::directory_entry& de);
        static bool ShouldIgnoreExt(const std::vector<std::string>& cfgList,
            const std::string& extNoDotLower);
        static std::string ToUtf8(const std::filesystem::path& p);
        std::string MakeRelativeAssetPathUtf8(const std::filesystem::path& abs) const;

    private:
        std::string           m_panelName   { "Assets" };
        std::string           m_rootPath    {"assets"};
        bool                  m_isVisible   { true };
        std::uint32_t         m_dockspaceId { 0 };
        AssetSelectedCallback m_onAssetSelected;

        std::filesystem::path              m_root;
        std::filesystem::path              m_current;
        std::vector<std::filesystem::path> m_hist;
        int                                m_histIndex{ -1 };

        std::vector<Entry> m_items;

        bool        m_searchMode{ false };
        std::string m_searchQuery;

        float m_tileSize      { 96.0f };
        float m_padding       { 8.0f };
        bool  m_gridView      { true };
        bool  m_showHidden    { false };
        bool  m_showExtensions{ true };

        ImGuiTableFlags m_listFlags =
            ImGuiTableFlags_SizingStretchSame   |
            ImGuiTableFlags_RowBg               |
            ImGuiTableFlags_Resizable           |
            ImGuiTableFlags_ScrollY;

        std::vector<std::string> m_ignoreExtensions{ ".scene", ".assets" };

        //~ Icons
        ImTextureID m_iconFolder{ 0ll };
        ImTextureID m_iconFile  { 0ll };
        ImTextureID m_iconImage { 0ll };
        ImTextureID m_iconMesh  { 0ll };
        ImTextureID m_iconShader{ 0ll };
        std::unordered_map<std::string, ImTextureID> m_extIcons;

        std::vector<char> m_dragBuf;
    };
}
