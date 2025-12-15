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
#include "engine/editor/widgets/creational_panel.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "engine/map/world.h"
#include "engine/system/registry/registry_scene.h"
#include "engine/system/registry/registry_light.h"

namespace
{
    constexpr ImVec2 kIcon16{ 24,24 };
    constexpr ImVec2 kIcon20{ 28,28 };

    inline bool SmallButtonWithId(const char* id, const char* label)
    {
        std::string text = std::string(label) + "##" + id;
        return ImGui::SmallButton(text.c_str());
    }

    inline bool IconButton(const char* id,
        ImTextureID tex,
        ImVec2 size,
        const char* fallback)
    {
        if (tex != 0ull)
        {
            return ImGui::ImageButton(id, tex, size);
        }
        return SmallButtonWithId(id, fallback);
    }

    inline bool RightAlignedIconButton(const char* id,
        ImTextureID tex,
        ImVec2 size,
        const char* fallback)
    {
        ImGuiStyle& st = ImGui::GetStyle();
        float w = (tex != 0ull)
            ? size.x + st.FramePadding.x * 2.0f
            : ImGui::CalcTextSize(fallback).x + st.FramePadding.x * 2.0f;

        float avail = ImGui::GetContentRegionAvail().x;
        if (avail < w + 2.0f)
        {
            ImGui::NewLine();
            avail = ImGui::GetContentRegionAvail().x;
        }

        float x = ImGui::GetCursorPosX() + avail - w;
        ImGui::SameLine();
        ImGui::SetCursorPosX(x);

        return IconButton(id, tex, size, fallback);
    }

    inline void RowIcon(ImTextureID tex)
    {
        if (tex != 0ull)
        {
            ImGui::Image(tex, kIcon16);
            ImGui::SameLine(0.0f, 6.0f);
        }
    }

    inline ImTextureID TryGetIcon(const kfe::ImguiCreationPanelCore::Config& cfg,
        const char* key)
    {
        auto it = cfg.icons.find(key);
        if (it == cfg.icons.end())
        {
            return 0ull;
        }
        return it->second;
    }

    static void BeginResultsTable()
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_TableHeaderBg));

        if (ImGui::BeginTable("##create_table",
            3,
            ImGuiTableFlags_Resizable
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 0.65f);
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch, 0.25f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 88.0f);
            ImGui::TableHeadersRow();
        }
    }

    static void EndResultsTable()
    {
        if (ImGui::GetCurrentTable())
        {
            ImGui::EndTable();
        }
        ImGui::PopStyleColor(3);
    }
} // namespace

kfe::ImguiCreationPanelCore::ImguiCreationPanelCore() noexcept
{
    m_search[0] = '\0';
}

void kfe::ImguiCreationPanelCore::SetConfig(const Config& cfg) noexcept
{
    m_config = cfg;
    m_icons.ready = false;
}

void kfe::ImguiCreationPanelCore::LoadIconsIfNeeded()
{
    if (m_icons.ready)
    {
        return;
    }

    m_icons.object = TryGetIcon(m_config, "icon.object");
    m_icons.light = TryGetIcon(m_config, "icon.light");
    m_icons.favOn = TryGetIcon(m_config, "icon.favorite_on");
    m_icons.favOff = TryGetIcon(m_config, "icon.favorite_off");
    m_icons.create = TryGetIcon(m_config, "icon.create");
    m_icons.clear = TryGetIcon(m_config, "icon.clear");

    m_icons.ready = true;
}

bool kfe::ImguiCreationPanelCore::InitFromRegistry()
{
    Clear();

    bool any = false;

    // Scene Objects
    {
        const auto& names = kfe::RegistrySceneObject::GetRegisteredNames();
        if (!names.empty())
        {
            any = true;

            Item base{};
            base.category = "Objects";
            base.tags = { "object", "scene", "entity" };

            for (const auto& n : names)
            {
                Item it = base;
                it.name = n;
                it.onCreate = [n](KFEWorld& world)
                    {
                        auto obj = kfe::RegistrySceneObject::Create(n);
                        if (!obj)
                            return;

                        world.AddSceneObject(std::move(obj));
                    };

                it.featured = false;
                Register(it);
            }
        }
    }

    // Lights
    {
        const auto& names = kfe::RegistryLights::GetRegisteredNames();
        if (!names.empty())
        {
            any = true;

            Item base{};
            base.category = "Lights";
            base.tags = { "light", "lighting", "shadow" };

            for (const auto& n : names)
            {
                Item it = base;
                it.name = n;

                it.onCreate = [n](KFEWorld& world)
                    {
                        auto light = kfe::RegistryLights::Create(n);
                        if (!light)
                            return;

                        world.AddLight(std::move(light));
                    };

                it.featured = false;
                Register(it);
            }
        }
    }

    return any;
}

void kfe::ImguiCreationPanelCore::Register(const Item& it)
{
    m_items.push_back(it);
}

void kfe::ImguiCreationPanelCore::Register(const std::vector<Item>& items)
{
    m_items.insert(m_items.end(), items.begin(), items.end());
}

void kfe::ImguiCreationPanelCore::Clear()
{
    m_items.clear();
    m_filtered.clear();
    m_recent.clear();
    m_favorites.clear();
    m_activeCategory.clear();
    m_selected = -1;
    m_search[0] = '\0';
}

// text helpers
std::string kfe::ImguiCreationPanelCore::LowerCopy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

bool kfe::ImguiCreationPanelCore::ContainsIC(const std::string& hay, const std::string& needle)
{
    if (needle.empty())
    {
        return true;
    }
    auto H = LowerCopy(hay);
    auto N = LowerCopy(needle);
    return H.find(N) != std::string::npos;
}

std::vector<std::string> kfe::ImguiCreationPanelCore::Tokenize(const char* q)
{
    std::vector<std::string> out;
    if (!q)
    {
        return out;
    }
    const char* p = q;
    while (*p)
    {
        while (*p && std::isspace((unsigned char)*p)) { ++p; }
        if (!*p) { break; }
        const char* b = p;
        while (*p && !std::isspace((unsigned char)*p)) { ++p; }
        out.emplace_back(b, p - b);
    }
    return out;
}

// scoring/filtering
int kfe::ImguiCreationPanelCore::ScoreItem(const Item& it, const std::vector<std::string>& tokens) const
{
    if (tokens.empty())
    {
        return (it.featured ? 1 : 0);
    }

    int score = 0;
    auto lname = LowerCopy(it.name);
    auto lcat = LowerCopy(it.category);

    for (auto& t : tokens)
    {
        auto lt = LowerCopy(t);
        bool matched{ false };

        if (lname.rfind(lt, 0) == 0)
        {
            score += 3;
            matched = true;
        }
        else if (lname.find(lt) != std::string::npos)
        {
            score += 2;
            matched = true;
        }

        if (!matched && !lcat.empty() && lcat.find(lt) != std::string::npos)
        {
            score += 1;
            matched = true;
        }

        if (!matched)
        {
            for (auto& tag : it.tags)
            {
                if (ContainsIC(tag, lt))
                {
                    score += 1;
                    break;
                }
            }
        }
    }
    return score;
}

void kfe::ImguiCreationPanelCore::BuildFiltered()
{
    const auto tokens = Tokenize(m_search);

    std::vector<ScoredIdx> scored;
    scored.reserve(m_items.size());

    for (int i = 0; i < (int)m_items.size(); ++i)
    {
        const auto& it = m_items[i];

        if (!m_activeCategory.empty())
        {
            if (!ContainsIC(it.category, m_activeCategory))
            {
                continue;
            }
        }

        const int s = ScoreItem(it, tokens);
        if (!tokens.empty() && s <= 0)
        {
            continue;
        }

        scored.push_back({ i, s });
    }

    std::sort(scored.begin(), scored.end(),
        [&](const ScoredIdx& a, const ScoredIdx& b)
        {
            if (a.score != b.score)
            {
                return a.score > b.score;
            }
            const auto& A = m_items[a.idx];
            const auto& B = m_items[b.idx];
            if (A.featured != B.featured)
            {
                return A.featured && !B.featured;
            }
            return A.name < B.name;
        });

    m_filtered.clear();
    m_filtered.reserve(scored.size());
    for (auto& s : scored)
    {
        m_filtered.push_back(s.idx);
    }

    if (m_selected >= (int)m_filtered.size())
    {
        m_selected = (int)m_filtered.size() - 1;
    }
}

void kfe::ImguiCreationPanelCore::HandleKeyboard(KFEWorld& world)
{
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            if (m_selected < (int)m_filtered.size() - 1) { ++m_selected; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            if (m_selected > 0) { --m_selected; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            if (m_selected >= 0 && m_selected < (int)m_filtered.size())
            {
                const auto& it = m_items[m_filtered[m_selected]];
                TriggerCreate(world, it);
            }
        }
    }
}

// toolbar
void kfe::ImguiCreationPanelCore::DrawToolbarLeft(ImguiCreationPanelCore* self)
{
    ImGui::SetNextItemWidth(-120.0f);
    if (ImGui::InputTextWithHint("##create_search", "Search…", self->m_search, IM_ARRAYSIZE(self->m_search)))
    {
        self->m_selected = 0;
    }

    ImGui::SameLine();
    if (IconButton("clear", self->m_icons.clear, kIcon20, "Clear"))
    {
        self->m_search[0] = '\0';
        self->m_selected = -1;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Show featured", &self->m_showFeatured);
}

// main draw
void kfe::ImguiCreationPanelCore::DrawCreation(KFEWorld& world)
{
    LoadIconsIfNeeded();

    ImGui::PushID(this);

    ImGui::BeginGroup();
    {
        DrawToolbarLeft(this);
    }
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float leftW = m_showCategories ? 160.0f : 0.0f;
    if (m_showCategories)
    {
        ImGui::BeginChild("##create_categories", ImVec2(leftW, 0), true);
        {
            DrawCategories();
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }

    ImGui::BeginChild("##create_results", ImVec2(0, 0), true);
    {
        HandleKeyboard(world);
        m_filtered.clear();
        BuildFiltered();

        const bool idle = (std::strlen(m_search) == 0) && m_activeCategory.empty();

        if (m_showFavorites && !m_favorites.empty() && idle)
        {
            if (ImGui::CollapsingHeader("Favorites", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawList(world, Section::Favorites);
            }
        }
        if (m_showFeatured && idle)
        {
            if (ImGui::CollapsingHeader("Featured", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawList(world, Section::Featured);
            }
        }
        if (ImGui::CollapsingHeader(idle ? "All" : "Results", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawList(world, Section::Results);
        }
        if (!m_recent.empty() && idle)
        {
            if (ImGui::CollapsingHeader("Recent", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawList(world, Section::Recent);
            }
        }
    }
    ImGui::EndChild();

    ImGui::PopID();
}

// list dispatch
void kfe::ImguiCreationPanelCore::DrawList(KFEWorld& world, Section sec)
{
    ImGui::PushID(static_cast<int>(sec));
    {
        switch (sec)
        {
        case Section::Results:   DrawResults(world);   break;
        case Section::Featured:  DrawFeatured(world);  break;
        case Section::Favorites: DrawFavorites(world); break;
        case Section::Recent:    DrawRecent(world);    break;
        }
    }
    ImGui::PopID();
}

// results table
void kfe::ImguiCreationPanelCore::DrawResults(KFEWorld& world)
{
    constexpr bool kCreateOnLeft = false;

    BeginResultsTable();

    for (int row = 0; row < (int)m_filtered.size(); ++row)
    {
        const int idx = m_filtered[row];
        const auto& it = m_items[idx];

        ImGui::PushID(row);
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            {
                bool fav = (m_favorites.count(it.name) != 0);
                ImTextureID favIcon = fav ? m_icons.favOn : m_icons.favOff;
                if (IconButton("fav", favIcon, kIcon16, fav ? "Unfav" : "Fav"))
                {
                    ToggleFavorite(it.name);
                }
                ImGui::SameLine(0.0f, 6.0f);

                ImTextureID catIcon = (it.category == "Lights") ? m_icons.light : m_icons.object;
                RowIcon(catIcon);

                if (kCreateOnLeft)
                {
                    if (IconButton("create_left", m_icons.create, kIcon16, "Create"))
                    {
                        TriggerCreate(world, it);
                    }
                    ImGui::SameLine(0.0f, 6.0f);
                }

                ImGuiSelectableFlags sflags = ImGuiSelectableFlags_AllowDoubleClick;
                const bool selected = (row == m_selected);
                if (ImGui::Selectable(it.name.c_str(), selected, sflags, ImVec2(0, 0)))
                {
                    m_selected = row;
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        TriggerCreate(world, it);
                    }
                }

                if (selected && ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    TriggerCreate(world, it);
                }
            }

            ImGui::TableNextColumn();
            {
                const char* cat = it.category.empty() ? "-" : it.category.c_str();
                ImGui::TextUnformatted(cat);

                ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                ImGui::SetCursorScreenPos(rect.Min);
                std::string cid = std::string("##cat_sel_") + std::to_string(row);
                bool catClick = ImGui::Selectable(cid.c_str(), false,
                    ImGuiSelectableFlags_AllowDoubleClick,
                    rect.GetSize());
                if (catClick && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    TriggerCreate(world, it);
                }
                ImGui::SetCursorScreenPos(rect.Max);
            }

            if (!kCreateOnLeft)
            {
                ImGui::TableNextColumn();
                {
                    if (RightAlignedIconButton("create", m_icons.create, kIcon20, "Create"))
                    {
                        TriggerCreate(world, it);
                    }
                }
            }
            else
            {
                ImGui::TableNextColumn();
            }

            if (ImGui::BeginPopupContextItem("row_ctx"))
            {
                if (ImGui::MenuItem("Create"))
                {
                    TriggerCreate(world, it);
                }
                bool isFav = m_favorites.count(it.name) != 0;
                if (ImGui::MenuItem(isFav ? "Unfavorite" : "Favorite"))
                {
                    ToggleFavorite(it.name);
                }
                ImGui::EndPopup();
            }
        }
        ImGui::PopID();
    }

    EndResultsTable();
}

// featured/favorites/recent
void kfe::ImguiCreationPanelCore::DrawFeatured(KFEWorld& world)
{
    for (const auto& it : m_items)
    {
        if (it.featured)
        {
            DrawCard(world, it, Section::Featured);
        }
    }
}

void kfe::ImguiCreationPanelCore::DrawFavorites(KFEWorld& world)
{
    for (const auto& it : m_items)
    {
        if (m_favorites.count(it.name))
        {
            DrawCard(world, it, Section::Favorites);
        }
    }
}

void kfe::ImguiCreationPanelCore::DrawRecent(KFEWorld& world)
{
    for (const auto& name : m_recent)
    {
        if (auto* it = FindByName(name))
        {
            DrawCard(world, *it, Section::Recent);
        }
    }
}

void kfe::ImguiCreationPanelCore::DrawCard(KFEWorld& world, const Item& it, Section sec)
{
    ImGui::PushID(static_cast<int>(sec));
    ImGui::PushID(it.name.c_str());
    {
        ImGui::BeginGroup();
        {
            ImTextureID catIcon = (it.category == "Lights") ? m_icons.light : m_icons.object;
            RowIcon(catIcon);
            ImGui::TextUnformatted(it.name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", it.category.empty() ? "-" : it.category.c_str());

            if (RightAlignedIconButton("create_card", m_icons.create, kIcon20, "Create"))
            {
                TriggerCreate(world, it);
            }
        }
        ImGui::EndGroup();
        ImGui::Separator();
    }
    ImGui::PopID();
    ImGui::PopID();
}

kfe::ImguiCreationPanelCore::Item* kfe::ImguiCreationPanelCore::FindByName(const std::string& n)
{
    for (auto& it : m_items)
    {
        if (it.name == n)
        {
            return &it;
        }
    }
    return nullptr;
}

void kfe::ImguiCreationPanelCore::PushRecent(const std::string& n)
{
    auto it = std::find(m_recent.begin(), m_recent.end(), n);
    if (it != m_recent.end())
    {
        m_recent.erase(it);
    }
    m_recent.push_front(n);
    if ((int)m_recent.size() > kMaxRecent)
    {
        m_recent.pop_back();
    }
}

void kfe::ImguiCreationPanelCore::TriggerCreate(KFEWorld& world, const Item& it)
{
    if (it.onCreate)
    {
        it.onCreate(world);
    }
    PushRecent(it.name);
}

void kfe::ImguiCreationPanelCore::ToggleFavorite(const std::string& name)
{
    if (m_favorites.count(name))
    {
        m_favorites.erase(name);
    }
    else
    {
        m_favorites.insert(name);
    }
}

void kfe::ImguiCreationPanelCore::DrawCategories()
{
    std::vector<std::string> cats;
    cats.reserve(16);
    cats.emplace_back("(All)");

    for (auto& it : m_items)
    {
        if (it.category.empty())
        {
            continue;
        }
        if (std::find(cats.begin(), cats.end(), it.category) == cats.end())
        {
            cats.push_back(it.category);
        }
    }
    std::sort(cats.begin() + 1, cats.end());

    if (ImGui::Selectable("(All)", m_activeCategory.empty()))
    {
        m_activeCategory.clear();
    }

    for (auto& c : cats)
    {
        if (c == "(All)")
        {
            continue;
        }
        bool sel = (m_activeCategory == c);
        if (ImGui::Selectable(c.c_str(), sel))
        {
            m_activeCategory = c;
            m_selected = 0;
        }
    }
}

bool kfe::KFECreationPanel::Initialize(KFEWorld* world)
{
    m_world = world;
    if (!m_world)
    {
        return false;
    }

    (void)m_core.InitFromRegistry();

    return true;
}

bool kfe::KFECreationPanel::Release()
{
    m_core.Clear();
    m_world = nullptr;
    return true;
}

void kfe::KFECreationPanel::Draw()
{
    if (!m_visible || !m_world)
    {
        return;
    }

    ImGuiWindowClass winClass{};
    winClass.DockNodeFlagsOverrideSet = 0;
    ImGui::SetNextWindowClass(&winClass);
    ImGui::SetNextWindowDockID(m_dockspaceId, ImGuiCond_Once);

    if (ImGui::Begin(m_title))
    {
        m_core.DrawCreation(*m_world);
    }
    ImGui::End();
}
