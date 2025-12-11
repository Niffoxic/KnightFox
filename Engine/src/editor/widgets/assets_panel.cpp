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
#include "engine/editor/widgets/assets_panel.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace fs = std::filesystem;

namespace
{
    static std::string ToLowerAscii(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    static void KmpBuildLps(const std::string& pat, std::vector<int>& lps)
    {
        int len = 0;
        lps.assign(static_cast<int>(pat.size()), 0);

        for (int i = 1; i < static_cast<int>(pat.size()); )
        {
            if (pat[i] == pat[len])
            {
                lps[i++] = ++len;
            }
            else if (len)
            {
                len = lps[len - 1];
            }
            else
            {
                lps[i++] = 0;
            }
        }
    }

    static bool KmpContainsICase(const std::string& haystack, const std::string& needle)
    {
        if (needle.empty()) return true;

        std::string h = ToLowerAscii(haystack);
        std::string p = ToLowerAscii(needle);

        std::vector<int> lps;
        KmpBuildLps(p, lps);

        int i = 0;
        int j = 0;

        while (i < static_cast<int>(h.size()))
        {
            if (h[i] == p[j])
            {
                ++i;
                ++j;
                if (j == static_cast<int>(p.size()))
                    return true;
            }
            else if (j)
            {
                j = lps[j - 1];
            }
            else
            {
                ++i;
            }
        }

        return false;
    }

    static std::string NormWithDot(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!s.empty() && s.front() != '.')
            s.insert(s.begin(), '.');
        return s;
    }
}

namespace kfe
{

    bool KFEAssetPanel::ShouldIgnoreExt(const std::vector<std::string>& cfgList,
        const std::string& extNoDotLower)
    {
        const std::string withDot = "." + extNoDotLower;
        for (const std::string& e : cfgList)
        {
            if (NormWithDot(e) == withDot)
                return true;
        }
        return false;
    }

    std::string KFEAssetPanel::ToUtf8(const fs::path& p)
    {
#if defined(_WIN32)
        auto u8 = p.u8string();
        return std::string(reinterpret_cast<const char*>(u8.c_str()));
#else
        return p.string();
#endif
    }

    bool KFEAssetPanel::IsHidden(const fs::directory_entry& de)
    {
#if defined(_WIN32)
        return de.path().filename().native().starts_with(L".");
#else
        const std::string name = de.path().filename().string();
        return !name.empty() && name[0] == '.';
#endif
    }

    std::string KFEAssetPanel::MakeRelativeAssetPathUtf8(const fs::path& abs) const
    {
        std::error_code ec;
        fs::path rel = fs::relative(abs, m_root, ec);
        std::string s = ec ? ToUtf8(abs) : ToUtf8(rel);

        for (char& c : s)
        {
            if (c == '\\') c = '/';
        }

        if (!m_rootPath.empty())
        {
            const std::string prefix = m_rootPath.back() == '/' ? m_rootPath : (m_rootPath + "/");
            if (!s.empty() && s.rfind(prefix, 0) != 0)
            {
                s = prefix + s;
            }
        }

        return s;
    }

    ImTextureID KFEAssetPanel::IconFor(const Entry& e) const
    {
        if (e.IsDir)
        {
            return m_iconFolder ? m_iconFolder : static_cast<ImTextureID>(0ll);
        }

        auto it = m_extIcons.find(e.Ext);
        if (it != m_extIcons.end() && it->second)
            return it->second;

        if (m_iconFile)
            return m_iconFile;

        return m_iconFolder;
    }

    KFEAssetPanel::KFEAssetPanel() noexcept
    {
    }

    KFEAssetPanel::KFEAssetPanel(_In_ const std::string& panelName) noexcept
        : m_panelName(panelName)
    {
    }

    KFEAssetPanel::~KFEAssetPanel() noexcept = default;

    KFEAssetPanel::KFEAssetPanel(KFEAssetPanel&& other) noexcept
        : m_panelName(std::move(other.m_panelName))
        , m_rootPath(std::move(other.m_rootPath))
        , m_isVisible(other.m_isVisible)
        , m_dockspaceId(other.m_dockspaceId)
        , m_onAssetSelected(std::move(other.m_onAssetSelected))
        , m_root(std::move(other.m_root))
        , m_current(std::move(other.m_current))
        , m_hist(std::move(other.m_hist))
        , m_histIndex(other.m_histIndex)
        , m_items(std::move(other.m_items))
        , m_searchMode(other.m_searchMode)
        , m_searchQuery(std::move(other.m_searchQuery))
        , m_tileSize(other.m_tileSize)
        , m_padding(other.m_padding)
        , m_gridView(other.m_gridView)
        , m_showHidden(other.m_showHidden)
        , m_showExtensions(other.m_showExtensions)
        , m_listFlags(other.m_listFlags)
        , m_ignoreExtensions(std::move(other.m_ignoreExtensions))
        , m_iconFolder(other.m_iconFolder)
        , m_iconFile(other.m_iconFile)
        , m_iconImage(other.m_iconImage)
        , m_iconMesh(other.m_iconMesh)
        , m_iconShader(other.m_iconShader)
        , m_extIcons(std::move(other.m_extIcons))
        , m_dragBuf(std::move(other.m_dragBuf))
    {
        other.m_dockspaceId = 0;
        other.m_isVisible = false;
        other.m_histIndex = -1;
        other.m_iconFolder = 0ll;
        other.m_iconFile = 0ll;
        other.m_iconImage = 0ll;
        other.m_iconMesh = 0ll;
        other.m_iconShader = 0ll;
    }

    KFEAssetPanel& KFEAssetPanel::operator=(KFEAssetPanel&& other) noexcept
    {
        if (this == &other) return *this;

        m_panelName = std::move(other.m_panelName);
        m_rootPath = std::move(other.m_rootPath);
        m_isVisible = other.m_isVisible;
        m_dockspaceId = other.m_dockspaceId;
        m_onAssetSelected = std::move(other.m_onAssetSelected);

        m_root = std::move(other.m_root);
        m_current = std::move(other.m_current);
        m_hist = std::move(other.m_hist);
        m_histIndex = other.m_histIndex;
        m_items = std::move(other.m_items);

        m_searchMode = other.m_searchMode;
        m_searchQuery = std::move(other.m_searchQuery);
        m_tileSize = other.m_tileSize;
        m_padding = other.m_padding;
        m_gridView = other.m_gridView;
        m_showHidden = other.m_showHidden;
        m_showExtensions = other.m_showExtensions;
        m_listFlags = other.m_listFlags;
        m_ignoreExtensions = std::move(other.m_ignoreExtensions);

        m_iconFolder = other.m_iconFolder;
        m_iconFile = other.m_iconFile;
        m_iconImage = other.m_iconImage;
        m_iconMesh = other.m_iconMesh;
        m_iconShader = other.m_iconShader;

        m_extIcons = std::move(other.m_extIcons);
        m_dragBuf = std::move(other.m_dragBuf);

        other.m_dockspaceId = 0;
        other.m_isVisible = false;
        other.m_histIndex = -1;
        other.m_iconFolder = 0ll;
        other.m_iconFile = 0ll;
        other.m_iconImage = 0ll;
        other.m_iconMesh = 0ll;
        other.m_iconShader = 0ll;

        return *this;
    }

    bool KFEAssetPanel::Initialize()
    {
        if (m_rootPath.empty())
        {
            m_rootPath = "Assets";
        }

        SetRootPath(m_rootPath);
        return true;
    }

    bool KFEAssetPanel::Release()
    {
        m_items.clear();
        m_hist.clear();
        m_dragBuf.clear();
        m_onAssetSelected = nullptr;
        return true;
    }

    _Use_decl_annotations_
    bool KFEAssetPanel::ParsePayload(const ImGuiPayload* payload, PayloadHeader& outHeader, std::string& outPathUtf8)
    {
        if (!payload || !payload->Data ||
            payload->DataSize < static_cast<int>(sizeof(PayloadHeader)))
            return false;

        if (std::string_view(payload->DataType) != std::string_view(kPayloadType))
            return false;

        const auto* hdr = reinterpret_cast<const PayloadHeader*>(payload->Data);
        if (hdr->magic != 0xA551'7BAD || hdr->version != 1)
            return false;

        const char* pathz = reinterpret_cast<const char*>(payload->Data) + sizeof(PayloadHeader);
        const char* end = reinterpret_cast<const char*>(payload->Data) + payload->DataSize;
        const char* zero = static_cast<const char*>(std::memchr(pathz, 0, end - pathz));
        if (!zero) return false;

        outHeader = *hdr;
        outPathUtf8 = pathz;
        return true;
    }

    void KFEAssetPanel::SetDockspaceId(std::uint32_t dockspaceId) noexcept
    {
        m_dockspaceId = dockspaceId;
    }

    std::uint32_t KFEAssetPanel::GetDockspaceId() const noexcept
    {
        return m_dockspaceId;
    }

    void KFEAssetPanel::SetVisible(bool visible) noexcept
    {
        m_isVisible = visible;
    }

    bool KFEAssetPanel::IsVisible() const noexcept
    {
        return m_isVisible;
    }

    void KFEAssetPanel::SetRootPath(_In_ const std::string& path)
    {
        m_rootPath = path;

        std::error_code ec;
        m_root = fs::weakly_canonical(fs::path(m_rootPath), ec);
        if (ec || !fs::exists(m_root))
        {
            m_root = fs::current_path();
        }

        m_current = m_root;
        m_hist.clear();
        m_hist.push_back(m_current);
        m_histIndex = 0;

        Rescan();
    }

    const std::string& KFEAssetPanel::GetRootPath() const noexcept
    {
        return m_rootPath;
    }

    void KFEAssetPanel::SetOnAssetSelected(_In_ AssetSelectedCallback callback)
    {
        m_onAssetSelected = std::move(callback);
    }

    const std::string& KFEAssetPanel::GetPanelName() const noexcept
    {
        return m_panelName;
    }

    void KFEAssetPanel::SetPanelName(_In_ const std::string& name)
    {
        m_panelName = name;
    }

    void KFEAssetPanel::SetIcons(ImTextureID folder, ImTextureID file) noexcept
    {
        m_iconFolder = folder;
        m_iconFile = file;
    }

    void KFEAssetPanel::SetExtIcon(std::string extNoDotLower, ImTextureID icon)
    {
        std::transform(extNoDotLower.begin(), extNoDotLower.end(), extNoDotLower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (!extNoDotLower.empty() && extNoDotLower.front() == '.')
            extNoDotLower.erase(extNoDotLower.begin());

        m_extIcons[extNoDotLower] = icon;
    }

    void KFEAssetPanel::Rescan()
    {
        if (m_searchMode)
        {
            RebuildItemsForSearch();
        }
        else
        {
            ScanDir(m_current);
        }
    }

    void KFEAssetPanel::GoUp()
    {
        if (m_current == m_root) return;
        Enter(m_current.parent_path());
    }

    void KFEAssetPanel::GoBack()
    {
        if (m_histIndex > 0)
        {
            --m_histIndex;
            m_current = m_hist[m_histIndex];
            Rescan();
        }
    }

    void KFEAssetPanel::GoForward()
    {
        if (m_histIndex + 1 < static_cast<int>(m_hist.size()))
        {
            ++m_histIndex;
            m_current = m_hist[m_histIndex];
            Rescan();
        }
    }

    void KFEAssetPanel::ScanDir(const fs::path& p)
    {
        m_items.clear();

        for (fs::directory_iterator it(p), end; it != end; ++it)
        {
            const fs::directory_entry& de = *it;

            if (!m_showHidden && IsHidden(de))
                continue;

            Entry e{};
            e.Path = de.path();
            e.IsDir = de.is_directory();

            e.Name = e.Path.filename().string();

            if (!e.IsDir)
            {
                std::string ext = e.Path.extension().string();
                if (!ext.empty() && ext[0] == '.')
                    ext.erase(ext.begin());

                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                e.Ext = std::move(ext);

                if (ShouldIgnoreExt(m_ignoreExtensions, e.Ext))
                    continue;

                std::error_code ec{};
                auto sz = fs::file_size(e.Path, ec);
                e.Size = ec ? 0ULL : static_cast<std::uint64_t>(sz);
            }

            e.Icon = IconFor(e);
            m_items.push_back(std::move(e));
        }
    }

    void KFEAssetPanel::Enter(const fs::path& child)
    {
        std::error_code ec;
        fs::path canon = fs::weakly_canonical(child, ec);
        if (ec || !fs::exists(canon) || !fs::is_directory(canon))
            return;

        fs::path rel = fs::relative(canon, m_root, ec);
        if (ec || rel.string().rfind("..", 0) == 0)
            return;

        m_current = canon;

        if (m_histIndex + 1 < static_cast<int>(m_hist.size()))
            m_hist.erase(m_hist.begin() + m_histIndex + 1, m_hist.end());

        m_hist.push_back(m_current);
        m_histIndex = static_cast<int>(m_hist.size()) - 1;

        if (m_searchMode)
            RebuildItemsForSearch();
        else
            ScanDir(m_current);
    }

    void KFEAssetPanel::RebuildItemsForSearch()
    {
        m_items.clear();
        m_searchMode = !m_searchQuery.empty();

        if (!m_searchMode)
        {
            ScanDir(m_current);
            return;
        }

        std::error_code ec;
        const std::string pat = m_searchQuery;

        for (fs::recursive_directory_iterator it(m_root,
            fs::directory_options::skip_permission_denied, ec), end;
            it != end; it.increment(ec))
        {
            const fs::path& p = it->path();

            if (!m_showHidden)
            {
#if defined(_WIN32)
                if (p.filename().native().starts_with(L".")) continue;
#else
                if (!p.filename().empty() && p.filename().c_str()[0] == '.') continue;
#endif
            }

            bool isDir = it->is_directory(ec);
            if (!isDir)
            {
                std::string ext = p.extension().string();
                if (!ext.empty() && ext[0] == '.')
                    ext.erase(ext.begin());

                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (ShouldIgnoreExt(m_ignoreExtensions, ext))
                    continue;
            }

            const std::string name = ToUtf8(p.filename());
            const std::string rel = MakeRelativeAssetPathUtf8(p);

            if (KmpContainsICase(name, pat) || KmpContainsICase(rel, pat))
            {
                Entry e{};
                e.Path = p;
                e.IsDir = isDir;
                e.Name = p.filename().string();

                if (!e.IsDir)
                {
                    std::string ext = p.extension().string();
                    if (!ext.empty() && ext[0] == '.')
                        ext.erase(ext.begin());

                    std::transform(ext.begin(), ext.end(), ext.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    e.Ext = std::move(ext);

                    std::error_code fec{};
                    auto sz = fs::file_size(p, fec);
                    e.Size = fec ? 0ULL : static_cast<std::uint64_t>(sz);
                }

                e.Icon = IconFor(e);
                m_items.push_back(std::move(e));
            }
        }
    }

    void KFEAssetPanel::Toolbar()
    {
        if (ImGui::BeginTable("asset_toolbar", 7,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX))
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (ImGui::Button("<")) { GoBack(); }

            ImGui::TableNextColumn();
            if (ImGui::Button(">")) { GoForward(); }

            ImGui::TableNextColumn();
            if (ImGui::Button("Up")) { GoUp(); }

            ImGui::TableNextColumn();
            if (ImGui::Button("Refresh")) { Rescan(); }

            ImGui::TableNextColumn();
            {
                std::error_code ec;
                fs::path rel = fs::relative(m_current, m_root, ec);
                std::string relStr = ec ? ToUtf8(m_current) : ToUtf8(rel);
                ImGui::TextUnformatted(relStr.c_str());
            }

            ImGui::TableNextColumn();
            {
                ImGui::SetNextItemWidth(220.0f);
                static char qbuf[256]{};

                if (!m_searchQuery.empty() && qbuf[0] == 0)
                {
                    const size_t n = std::min(m_searchQuery.size(), sizeof(qbuf) - 1);
                    std::memcpy(qbuf, m_searchQuery.data(), n);
                    qbuf[n] = 0;
                }

                bool changed = ImGui::InputTextWithHint("##asset_search_live",
                    "Search...", qbuf, sizeof(qbuf));
                if (changed)
                {
                    m_searchQuery = qbuf;
                    m_searchMode = !m_searchQuery.empty();
                    if (m_searchMode) RebuildItemsForSearch();
                    else              ScanDir(m_current);
                }
            }

            ImGui::TableNextColumn();
            if (ImGui::Button("Clear"))
            {
                if (!m_searchQuery.empty())
                {
                    m_searchQuery.clear();
                    m_searchMode = false;
                    ScanDir(m_current);
                }
            }

            ImGui::EndTable();
        }

        if (m_searchMode)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("Results: %zu", m_items.size());
        }

        // View toggle
        ImGui::SameLine();
        if (ImGui::RadioButton("Grid", m_gridView)) m_gridView = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("List", !m_gridView)) m_gridView = false;
    }

    void KFEAssetPanel::Breadcrumbs()
    {
        if (ImGui::Button("Assets"))
        {
            Enter(m_root);
        }

        ImGui::SameLine();

        fs::path rel;
        std::vector<std::string> parts;

        for (const auto& comp : m_current.lexically_relative(m_root))
            parts.push_back(ToUtf8(comp));

        for (size_t i = 0; i < parts.size(); ++i)
        {
            rel /= parts[i];

            ImGui::SameLine();
            if (ImGui::Button(parts[i].c_str()))
            {
                Enter(m_root / rel);
            }

            if (i + 1 < parts.size())
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("/");
            }
        }
    }

    void KFEAssetPanel::DrawGrid()
    {
        const float cell = m_tileSize + m_padding;
        const float avail = ImGui::GetContentRegionAvail().x;
        int         columns = std::clamp(static_cast<int>(std::floor(avail / cell)), 1, 32);

        fs::path pendingNav;

        if (ImGui::BeginTable("asset_grid", columns,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX))
        {
            int idx = 0;

            for (auto& e : m_items)
            {
                ImGui::TableNextColumn();
                ImGui::PushID(idx++);

                ImVec2 sz{ m_tileSize, m_tileSize };
                bool activated = false;

                if (e.Icon)
                {
                    activated = ImGui::ImageButton("icon", e.Icon, sz);
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1));
                    activated = ImGui::Button(e.IsDir ? "[DIR]" : "[FILE]", sz);
                    ImGui::PopStyleColor();
                }

                if (activated)
                {
                    if (e.IsDir)
                    {
                        pendingNav = e.Path;
                    }
                    else if (m_onAssetSelected)
                    {
                        const std::string relStr = MakeRelativeAssetPathUtf8(e.Path);
                        m_onAssetSelected(relStr);
                    }
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    m_dragBuf.clear();

                    PayloadHeader hdr{};
                    hdr.kind = static_cast<std::uint16_t>(
                        e.IsDir ? Kind::Directory : Kind::File);

                    const std::string relStr = MakeRelativeAssetPathUtf8(e.Path);

                    m_dragBuf.resize(sizeof(PayloadHeader) + relStr.size() + 1);
                    std::memcpy(m_dragBuf.data(), &hdr, sizeof(PayloadHeader));
                    std::memcpy(m_dragBuf.data() + sizeof(PayloadHeader),
                        relStr.c_str(), relStr.size() + 1);

                    ImGui::SetDragDropPayload(kPayloadType,
                        m_dragBuf.data(),
                        static_cast<int>(m_dragBuf.size()));

                    ImGui::TextUnformatted(relStr.c_str());
                    ImGui::EndDragDropSource();
                }

                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + m_tileSize);
                if (m_showExtensions || e.IsDir)
                {
                    ImGui::TextUnformatted(e.Name.c_str());
                }
                else
                {
                    const size_t dot = e.Name.rfind('.');
                    if (dot == std::string::npos)
                    {
                        ImGui::TextUnformatted(e.Name.c_str());
                    }
                    else
                    {
                        ImGui::TextUnformatted(e.Name.substr(0, dot).c_str());
                    }
                }
                ImGui::PopTextWrapPos();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        if (!pendingNav.empty())
        {
            Enter(pendingNav);
        }
    }

    void KFEAssetPanel::DrawList()
    {
        fs::path pendingNav;

        if (ImGui::BeginTable("asset_list", 3, m_listFlags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            int idx = 0;
            for (auto& e : m_items)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(idx++);

                if (e.Icon)
                {
                    ImGui::Image(e.Icon, ImVec2(16, 16));
                    ImGui::SameLine();
                }

                if (ImGui::Selectable(e.Name.c_str(), false,
                    ImGuiSelectableFlags_SpanAllColumns))
                {
                    if (e.IsDir)
                    {
                        pendingNav = e.Path;
                    }
                    else if (m_onAssetSelected)
                    {
                        const std::string relStr = MakeRelativeAssetPathUtf8(e.Path);
                        m_onAssetSelected(relStr);
                    }
                }

                // Drag & drop source
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    m_dragBuf.clear();

                    PayloadHeader hdr{};
                    hdr.kind = static_cast<std::uint16_t>(
                        e.IsDir ? Kind::Directory : Kind::File);

                    const std::string relStr = MakeRelativeAssetPathUtf8(e.Path);

                    m_dragBuf.resize(sizeof(PayloadHeader) + relStr.size() + 1);
                    std::memcpy(m_dragBuf.data(), &hdr, sizeof(PayloadHeader));
                    std::memcpy(m_dragBuf.data() + sizeof(PayloadHeader),
                        relStr.c_str(), relStr.size() + 1);

                    ImGui::SetDragDropPayload(kPayloadType,
                        m_dragBuf.data(),
                        static_cast<int>(m_dragBuf.size()));

                    ImGui::TextUnformatted(relStr.c_str());
                    ImGui::EndDragDropSource();
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(e.IsDir
                    ? "Folder"
                    : (e.Ext.empty() ? "File" : e.Ext.c_str()));

                ImGui::TableNextColumn();
                if (!e.IsDir)
                {
                    ImGui::Text("%llu",
                        static_cast<unsigned long long>(e.Size));
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        if (!pendingNav.empty())
        {
            Enter(pendingNav);
        }
    }

    void KFEAssetPanel::Draw()
    {
        if (!m_isVisible)
        {
            return;
        }

        if (m_dockspaceId != 0)
        {
            ImGui::SetNextWindowDockID(static_cast<ImGuiID>(m_dockspaceId),
                ImGuiCond_FirstUseEver);
        }

        if (!ImGui::Begin(m_panelName.c_str(), nullptr))
        {
            ImGui::End();
            return;
        }

        Toolbar();
        Breadcrumbs();

        ImGui::Separator();

        if (m_gridView)
            DrawGrid();
        else
            DrawList();

        ImGui::End();
    }

}
