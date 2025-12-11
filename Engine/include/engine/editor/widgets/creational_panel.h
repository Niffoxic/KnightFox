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

#include "imgui/imgui.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <functional>

namespace kfe
{
    class KFEWorld;
    class IKFESceneObject;

    class ImguiCreationPanelCore
    {
    public:
        struct Item
        {
            std::string name;
            std::string category;
            std::vector<std::string> tags;
            bool featured{ false };
            std::function<void(KFEWorld&)> onCreate;
        };

        struct Config
        {
            // Keys:
            //  - "icon.object"
            //  - "icon.favorite_on"
            //  - "icon.favorite_off"
            //  - "icon.create"
            //  - "icon.clear"
            std::unordered_map<std::string, ImTextureID> icons;
        };

        ImguiCreationPanelCore() noexcept;

        void SetConfig(const Config& cfg) noexcept;
        bool InitFromRegistry();

        void Register(const Item& it);
        void Register(const std::vector<Item>& items);
        void Clear();

        void DrawCreation(KFEWorld& world);

    private:
        struct ScoredIdx
        {
            int idx{ -1 };
            int score{ 0 };
        };

        enum class Section
        {
            Results = 0,
            Featured,
            Favorites,
            Recent
        };

        struct Icons
        {
            ImTextureID object{ 0ull };
            ImTextureID favOn{ 0ull };
            ImTextureID favOff{ 0ull };
            ImTextureID create{ 0ull };
            ImTextureID clear{ 0ull };
            bool        ready{ false };
        };

        // text helpers
        static std::string              LowerCopy(std::string s);
        static bool                     ContainsIC(const std::string& hay, const std::string& needle);
        static std::vector<std::string> Tokenize(const char* q);

        // data ops
        Item* FindByName(const std::string& n);
        void  PushRecent(const std::string& n);
        void  TriggerCreate(KFEWorld& world, const Item& it);
        void  ToggleFavorite(const std::string& name);

        // filtering
        int  ScoreItem(const Item& it, const std::vector<std::string>& tokens) const;
        void BuildFiltered();
        void HandleKeyboard(KFEWorld& world);

        // UI
        void LoadIconsIfNeeded();
        static void DrawToolbarLeft(ImguiCreationPanelCore* self);

        void DrawList(KFEWorld& world, Section sec);
        void DrawResults(KFEWorld& world);
        void DrawFeatured(KFEWorld& world);
        void DrawFavorites(KFEWorld& world);
        void DrawRecent(KFEWorld& world);
        void DrawCard(KFEWorld& world, const Item& it, Section sec);
        void DrawCategories();

    private:
        Config                          m_config{};
        Icons                           m_icons{};

        std::vector<Item>               m_items;
        std::vector<int>                m_filtered;
        std::deque<std::string>         m_recent;
        std::unordered_set<std::string> m_favorites;

        std::string                     m_activeCategory;
        bool                            m_showCategories{ true };
        bool                            m_showFeatured{ true };
        bool                            m_showFavorites{ true };

        int                             m_selected{ -1 };
        static constexpr int            kMaxRecent = 16;

        char                            m_search[128]{};
    };

    class KFE_API KFECreationPanel
    {
    public:
        KFECreationPanel() = default;
        ~KFECreationPanel() = default;

        KFECreationPanel(const KFECreationPanel&) = delete;
        KFECreationPanel& operator=(const KFECreationPanel&) = delete;

        KFECreationPanel(KFECreationPanel&&) = default;
        KFECreationPanel& operator=(KFECreationPanel&&) = default;

        void SetPanelName(const char* name) noexcept { m_title = name ? name : "Creation"; }
        void SetDockspaceId(ImGuiID id) noexcept { m_dockspaceId = id; }
        void SetVisible(bool v) noexcept { m_visible = v; }

        [[nodiscard]] bool Initialize(KFEWorld* world);
        [[nodiscard]] bool Release();

        void Draw();

        ImguiCreationPanelCore& GetCore() noexcept { return m_core; }

    private:
        const char* m_title{ "Creation" };
        ImGuiID                m_dockspaceId{ 0 };
        bool                   m_visible{ false };
        KFEWorld* m_world{ nullptr };
        ImguiCreationPanelCore m_core{};
    };
} // namespace kfe
