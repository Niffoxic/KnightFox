#pragma once
#include "EngineAPI.h"
#include "engine/core.h"
#include "engine/utils/helpers.h"

#include "engine/render_manager/api/buffer/buffer.h"
#include "engine/render_manager/api/buffer/constant_buffer.h"
#include "engine/render_manager/api/components/device.h"
#include "engine/render_manager/api/heap/heap_cbv_srv_uav.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace kfe
{
    struct KFE_FRAME_CONSTANT_BUFFER_DESC
    {
        KFEDevice* Device{ nullptr };
        KFEResourceHeap* ResourceHeap{ nullptr };
        std::uint32_t    FrameCount{ 0u };
        std::uint32_t    SizeInBytes{ 0u };
    };

    class KFE_API KFEFrameConstantBuffer final
    {
    public:
        KFEFrameConstantBuffer() = default;
        ~KFEFrameConstantBuffer() = default;

        KFEFrameConstantBuffer(const KFEFrameConstantBuffer&) = delete;
        KFEFrameConstantBuffer(KFEFrameConstantBuffer&&) noexcept = default;

        KFEFrameConstantBuffer& operator=(const KFEFrameConstantBuffer&) = delete;
        KFEFrameConstantBuffer& operator=(KFEFrameConstantBuffer&&) noexcept = default;

        NODISCARD bool Initialize(_In_ const KFE_FRAME_CONSTANT_BUFFER_DESC& desc) noexcept;
        NODISCARD bool Destroy() noexcept;

        void Step() noexcept;

        NODISCARD bool IsInitialized() const noexcept;

        NODISCARD std::uint32_t GetFrameCount() const noexcept;
        NODISCARD std::uint32_t GetFrameIndex() const noexcept;
        NODISCARD std::uint32_t GetSizeInBytes() const noexcept;

        NODISCARD KFEBuffer* GetBuffer() const noexcept;
        NODISCARD KFEConstantBuffer* GetView() const noexcept;

        NODISCARD void* GetMappedData() const noexcept;

        template<class T>
        inline T* GetMappedAs() const noexcept
        {
            return static_cast<T*>(GetMappedData());
        }

        template<class T>
        inline void Update(_In_ const T& data) noexcept
        {
            T* dst = GetMappedAs<T>();
            if (!dst) return;
            *dst = data;
        }

    private:
        struct Slice
        {
            std::unique_ptr<KFEBuffer>         Buffer{};
            std::unique_ptr<KFEConstantBuffer> View{};
        };

        NODISCARD bool CreateSlices(_In_ const KFE_FRAME_CONSTANT_BUFFER_DESC& desc) noexcept;

    private:
        std::vector<Slice> m_slices{};
        std::uint32_t      m_frameCount{ 0u };
        std::uint32_t      m_frameIndex{ 0u };
        std::uint32_t      m_sizeBytes{ 0u };
        bool               m_initialized{ false };
    };

} // namespace kfe
