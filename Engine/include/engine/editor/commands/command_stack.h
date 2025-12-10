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

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <functional>

#include "interface_command.h"

namespace kfe
{
    class MapEditorContext;

    class KFE_API KFECommandStack
    {
    public:
        KFECommandStack() noexcept;
        explicit KFECommandStack(std::size_t maxDepth) noexcept;

        KFECommandStack           (KFECommandStack&& other) noexcept;
        KFECommandStack& operator=(KFECommandStack&& other) noexcept;

        KFECommandStack           (const KFECommandStack&) = delete;
        KFECommandStack& operator=(const KFECommandStack&) = delete;

        ~KFECommandStack() = default;

        void Execute(std::unique_ptr<IKFECommand> cmd, MapEditorContext* ctx);
        bool Undo(MapEditorContext* ctx);
        bool Redo(MapEditorContext* ctx);

        void Clear() noexcept;
        void ShrinkToFit();

        void        SetMaxDepth(std::size_t depth) noexcept;
        std::size_t MaxDepth() const noexcept;

        bool        CanUndo() const noexcept;
        bool        CanRedo() const noexcept;
        std::size_t UndoDepth() const noexcept;
        std::size_t RedoDepth() const noexcept;

        const std::vector<std::unique_ptr<IKFECommand>>& Done()   const noexcept;
        const std::vector<std::unique_ptr<IKFECommand>>& Undone() const noexcept;

    private:
        void TrimIfNeeded() noexcept;

        std::vector<std::unique_ptr<IKFECommand>> m_done;
        std::vector<std::unique_ptr<IKFECommand>> m_undone;
        std::size_t m_maxDepth{ 256 };
    };
}
