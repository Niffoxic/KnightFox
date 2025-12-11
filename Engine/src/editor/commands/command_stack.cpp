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
#include "engine/editor/commands/command_stack.h"

namespace kfe
{
    KFECommandStack::KFECommandStack() noexcept
        : m_done()
        , m_undone()
        , m_maxDepth(256)
    {
    }

    KFECommandStack::KFECommandStack(std::size_t maxDepth) noexcept
        : m_done()
        , m_undone()
        , m_maxDepth(maxDepth)
    {
    }

    KFECommandStack::KFECommandStack(KFECommandStack&& other) noexcept
        : m_done(std::move(other.m_done))
        , m_undone(std::move(other.m_undone))
        , m_maxDepth(other.m_maxDepth)
    {
        other.m_maxDepth = 0;
    }

    KFECommandStack& KFECommandStack::operator=(KFECommandStack&& other) noexcept
    {
        if (this != &other)
        {
            m_done = std::move(other.m_done);
            m_undone = std::move(other.m_undone);
            m_maxDepth = other.m_maxDepth;
            other.m_maxDepth = 0;
        }
        return *this;
    }

    void KFECommandStack::Execute(std::unique_ptr<IKFECommand> cmd, MapEditorContext* ctx)
    {
        if (!cmd)
            return;

        cmd->Execute(ctx);
        m_done.push_back(std::move(cmd));
        m_undone.clear();
        TrimIfNeeded();
    }

    bool KFECommandStack::Undo(MapEditorContext* ctx)
    {
        if (!CanUndo())
            return false;

        auto& cmd = m_done.back();
        cmd->Undo(ctx);
        m_undone.push_back(std::move(cmd));
        m_done.pop_back();
        return true;
    }

    bool KFECommandStack::Redo(MapEditorContext* ctx)
    {
        if (!CanRedo())
            return false;

        auto& cmd = m_undone.back();
        cmd->Execute(ctx);
        m_done.push_back(std::move(cmd));
        m_undone.pop_back();
        TrimIfNeeded();
        return true;
    }

    void KFECommandStack::Clear() noexcept
    {
        m_done.clear();
        m_undone.clear();
    }

    void KFECommandStack::ShrinkToFit()
    {
        m_done.shrink_to_fit();
        m_undone.shrink_to_fit();
    }

    void KFECommandStack::SetMaxDepth(std::size_t depth) noexcept
    {
        m_maxDepth = depth;
        TrimIfNeeded();
    }

    std::size_t KFECommandStack::MaxDepth() const noexcept
    {
        return m_maxDepth;
    }

    bool KFECommandStack::CanUndo() const noexcept
    {
        return !m_done.empty();
    }

    bool KFECommandStack::CanRedo() const noexcept
    {
        return !m_undone.empty();
    }

    std::size_t KFECommandStack::UndoDepth() const noexcept
    {
        return m_done.size();
    }

    std::size_t KFECommandStack::RedoDepth() const noexcept
    {
        return m_undone.size();
    }

    const std::vector<std::unique_ptr<IKFECommand>>& KFECommandStack::Done() const noexcept
    {
        return m_done;
    }

    const std::vector<std::unique_ptr<IKFECommand>>& KFECommandStack::Undone() const noexcept
    {
        return m_undone;
    }

    void KFECommandStack::TrimIfNeeded() noexcept
    {
        if (m_maxDepth == 0)
        {
            m_done.clear();
            return;
        }

        if (m_done.size() <= m_maxDepth)
            return;

        const std::size_t overflow = m_done.size() - m_maxDepth;
        if (overflow > 0)
            m_done.erase(m_done.begin(), m_done.begin() + static_cast<std::ptrdiff_t>(overflow));
    }
}
