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
#include "engine/render_manager/api/root_signature.h"
#include "engine/render_manager/api/components/device.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <vector>

#include "engine/utils/logger.h"

using Microsoft::WRL::ComPtr;

#pragma region Impl_Definition

class kfe::KFERootSignature::Impl
{
public:
    Impl() = default;
    ~Impl();

    NODISCARD bool Initialize(_In_ const KFE_RG_CREATE_DESC& desc) noexcept;

    NODISCARD bool InitializeFromSerialized(
        _In_ KFEDevice* device,
        _In_ const void* blob,
        _In_ size_t         blobSize,
        _In_ const wchar_t* debugName = nullptr) noexcept;

    NODISCARD bool InitializeFromFile(
        _In_ KFEDevice* device,
        _In_ const wchar_t* filePath,
        _In_ const wchar_t* debugName = nullptr) noexcept;

    NODISCARD bool Destroy() noexcept;
    void Reset() noexcept;

    NODISCARD bool  IsInitialized() const noexcept;
    NODISCARD void* GetNative()       noexcept;
    NODISCARD const void* GetNative() const noexcept;

    NODISCARD KFE_RG_CREATE_DESC GetCreateDesc() const noexcept;

    void Swap(_In_ KFERootSignature& other) noexcept;

    void                          SetDebugName(_In_opt_ const wchar_t* name) noexcept;
    NODISCARD const std::wstring& GetDebugName() const noexcept;

private:
    ComPtr<ID3D12RootSignature> m_rootSignature{ nullptr };
    KFE_RG_CREATE_DESC          m_desc{};
    std::wstring                m_debugName;
};

#pragma endregion

#pragma region RootSignature_Body

kfe::KFERootSignature::KFERootSignature()
    : m_impl(std::make_unique<Impl>())
{
}

kfe::KFERootSignature::~KFERootSignature()
{
    if (m_impl)
    {
        if (!m_impl->Destroy())
        {
            LOG_ERROR("KFERootSignature::~KFERootSignature: Failed to destroy root signature.");
        }
    }
}

kfe::KFERootSignature::KFERootSignature(KFERootSignature&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

kfe::KFERootSignature& kfe::KFERootSignature::operator=(KFERootSignature&& other) noexcept
{
    if (this != &other)
    {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Initialize(const KFE_RG_CREATE_DESC& desc) noexcept
{
    if (!m_impl)
    {
        m_impl = std::make_unique<Impl>();
    }
    return m_impl->Initialize(desc);
}

_Use_decl_annotations_
bool kfe::KFERootSignature::InitializeFromSerialized(
    _In_ KFEDevice* device,
    _In_ const void* blob,
    _In_ size_t         blobSize,
    _In_ const wchar_t* debugName) noexcept
{
    if (!m_impl)
    {
        m_impl = std::make_unique<Impl>();
    }
    return m_impl->InitializeFromSerialized(device, blob, blobSize, debugName);
}

_Use_decl_annotations_
bool kfe::KFERootSignature::InitializeFromFile(
    _In_ KFEDevice* device,
    _In_ const wchar_t* filePath,
    _In_ const wchar_t* debugName) noexcept
{
    if (!m_impl)
    {
        m_impl = std::make_unique<Impl>();
    }
    return m_impl->InitializeFromFile(device, filePath, debugName);
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Destroy() noexcept
{
    if (!m_impl)
    {
        return true;
    }
    return m_impl->Destroy();
}

void kfe::KFERootSignature::Reset() noexcept
{
    if (m_impl)
    {
        m_impl->Reset();
    }
}

_Use_decl_annotations_
bool kfe::KFERootSignature::IsInitialized() const noexcept
{
    return m_impl && m_impl->IsInitialized();
}

_Use_decl_annotations_
void* kfe::KFERootSignature::GetNative() noexcept
{
    if (!m_impl)
    {
        return nullptr;
    }
    return m_impl->GetNative();
}

_Use_decl_annotations_
const void* kfe::KFERootSignature::GetNative() const noexcept
{
    if (!m_impl)
    {
        return nullptr;
    }
    return m_impl->GetNative();
}

_Use_decl_annotations_
kfe::KFE_RG_CREATE_DESC kfe::KFERootSignature::GetCreateDesc() const noexcept
{
    if (!m_impl)
    {
        return {};
    }
    return m_impl->GetCreateDesc();
}

void kfe::KFERootSignature::SetDebugName(_In_opt_ const wchar_t* name) noexcept
{
    if (!m_impl)
    {
        return;
    }
    m_impl->SetDebugName(name);
}

_Use_decl_annotations_
const std::wstring& kfe::KFERootSignature::GetDebugName() const noexcept
{
    static const std::wstring empty;
    if (!m_impl)
    {
        return empty;
    }
    return m_impl->GetDebugName();
}

_Use_decl_annotations_
void kfe::KFERootSignature::Swap(KFERootSignature& other) noexcept
{
    std::swap(m_impl, other.m_impl);
}

#pragma endregion

#pragma region Impl_Body

kfe::KFERootSignature::Impl::~Impl()
{
    if (!Destroy())
    {
        LOG_ERROR("KFERootSignature::Impl::~Impl: Failed to destroy root signature.");
    }
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Impl::Initialize(const KFE_RG_CREATE_DESC& desc) noexcept
{
    if (!desc.Device)
    {
        LOG_ERROR("KFERootSignature::Impl::Initialize: Device is null.");
        return false;
    }

    ID3D12Device* nativeDevice = desc.Device->GetNative();
    if (!nativeDevice)
    {
        LOG_ERROR("KFERootSignature::Impl::Initialize: Native device is null.");
        return false;
    }

    if (desc.NumRootParameters > 0 && !desc.RootParameters)
    {
        LOG_ERROR("KFERootSignature::Impl::Initialize: Root parameters pointer is null but NumRootParameters > 0.");
        return false;
    }

    if (desc.NumStaticSamplers > 0 && !desc.StaticSamplers)
    {
        LOG_ERROR("KFERootSignature::Impl::Initialize: Static samplers pointer is null but NumStaticSamplers > 0.");
        return false;
    }

    const D3D12_ROOT_PARAMETER* rootParams = desc.RootParameters;
    const D3D12_STATIC_SAMPLER_DESC* staticSamplers = desc.StaticSamplers;

    // Fill 1.0-style root signature desc
    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = desc.NumRootParameters;
    rsDesc.pParameters = rootParams;
    rsDesc.NumStaticSamplers = desc.NumStaticSamplers;
    rsDesc.pStaticSamplers = staticSamplers;
    rsDesc.Flags = desc.Flags;

    // Check highest version supported
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData{};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    HRESULT hrFeature = nativeDevice->CheckFeatureSupport(
        D3D12_FEATURE_ROOT_SIGNATURE,
        &featureData,
        sizeof(featureData));

    if (FAILED(hrFeature))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rsDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob);

    if (FAILED(hr) || !signatureBlob)
    {
        if (errorBlob)
        {
            const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
            LOG_ERROR("KFERootSignature::Impl::Initialize: D3D12SerializeRootSignature failed (hr=0x{:08X}): {}",
                static_cast<unsigned int>(hr),
                msg ? msg : "(null error msg)");
        }
        else
        {
            LOG_ERROR("KFERootSignature::Impl::Initialize: D3D12SerializeRootSignature failed (hr=0x{:08X}) with no error blob.",
                static_cast<unsigned int>(hr));
        }
        return false;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = nativeDevice->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));

    if (FAILED(hr))
    {
        LOG_ERROR("KFERootSignature::Impl::Initialize: CreateRootSignature failed (hr=0x{:08X}).",
            static_cast<unsigned int>(hr));
        return false;
    }

    // Destroy previous, if any
    if (!Destroy())
    {
        LOG_ERROR("KFERootSignature::Impl::Initialize: Destroy failed before assigning new root signature.");
    }

    m_rootSignature = std::move(rootSig);
    m_desc = desc;

    if (!m_debugName.empty())
    {
        m_rootSignature->SetName(m_debugName.c_str());
    }

    return true;
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Impl::InitializeFromSerialized(
    _In_ KFEDevice* device,
    _In_ const void* blob,
    _In_ size_t         blobSize,
    _In_ const wchar_t* debugName) noexcept
{
    if (!device || !blob || blobSize == 0)
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromSerialized: Invalid args.");
        return false;
    }

    ID3D12Device* nativeDevice = device->GetNative();
    if (!nativeDevice)
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromSerialized: Native device is null.");
        return false;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    HRESULT hr = nativeDevice->CreateRootSignature(
        0,
        blob,
        blobSize,
        IID_PPV_ARGS(&rootSig));

    if (FAILED(hr))
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromSerialized: CreateRootSignature failed (hr=0x{:08X}).",
            static_cast<unsigned int>(hr));
        return false;
    }

    if (!Destroy())
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromSerialized: Destroy failed before assigning new root signature.");
    }

    m_rootSignature = std::move(rootSig);
    m_desc = {};
    m_desc.Device = device;

    SetDebugName(debugName);

    return true;
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Impl::InitializeFromFile(
    _In_ KFEDevice* device,
    _In_ const wchar_t* filePath,
    _In_ const wchar_t* debugName) noexcept
{
    if (!device || !filePath)
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromFile: Invalid args.");
        return false;
    }

    HANDLE hFile = ::CreateFileW(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromFile: Failed to open file.");
        return false;
    }

    LARGE_INTEGER fileSize{};
    if (!::GetFileSizeEx(hFile, &fileSize))
    {
        ::CloseHandle(hFile);
        LOG_ERROR("KFERootSignature::Impl::InitializeFromFile: GetFileSizeEx failed.");
        return false;
    }

    if (fileSize.QuadPart <= 0 ||
        static_cast<ULONGLONG>(fileSize.QuadPart) > static_cast<ULONGLONG>(SIZE_MAX))
    {
        ::CloseHandle(hFile);
        LOG_ERROR("KFERootSignature::Impl::InitializeFromFile: Invalid file size.");
        return false;
    }

    std::vector<std::uint8_t> buffer(static_cast<size_t>(fileSize.QuadPart));

    DWORD bytesRead = 0;
    BOOL readOk = ::ReadFile(
        hFile,
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        &bytesRead,
        nullptr);

    ::CloseHandle(hFile);

    if (!readOk || bytesRead != buffer.size())
    {
        LOG_ERROR("KFERootSignature::Impl::InitializeFromFile: ReadFile failed.");
        return false;
    }

    return InitializeFromSerialized(device, buffer.data(), buffer.size(), debugName);
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Impl::Destroy() noexcept
{
    Reset();
    return true;
}

void kfe::KFERootSignature::Impl::Reset() noexcept
{
    m_rootSignature.Reset();
    m_desc = {};
    m_debugName.clear();
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Impl::IsInitialized() const noexcept
{
    return m_rootSignature != nullptr;
}

_Use_decl_annotations_
void* kfe::KFERootSignature::Impl::GetNative() noexcept
{
    return m_rootSignature.Get();
}

_Use_decl_annotations_
const void* kfe::KFERootSignature::Impl::GetNative() const noexcept
{
    return m_rootSignature.Get();
}

_Use_decl_annotations_
kfe::KFE_RG_CREATE_DESC kfe::KFERootSignature::Impl::GetCreateDesc() const noexcept
{
    return m_desc;
}

_Use_decl_annotations_
void kfe::KFERootSignature::Impl::SetDebugName(_In_opt_ const wchar_t* name) noexcept
{
    if (!name || *name == L'\0')
    {
        m_debugName.clear();
        if (m_rootSignature)
        {
            const HRESULT hr = m_rootSignature->SetName(nullptr);
            if (FAILED(hr))
            {
                LOG_ERROR("KFERootSignature::Impl::SetDebugName: Failed to clear name.");
            }
        }
        return;
    }

    m_debugName = name;
    if (m_rootSignature)
    {
        const HRESULT hr = m_rootSignature->SetName(m_debugName.c_str());
        if (FAILED(hr))
        {
            LOG_ERROR("KFERootSignature::Impl::SetDebugName: SetName failed.");
        }
    }
}

_Use_decl_annotations_
const std::wstring& kfe::KFERootSignature::Impl::GetDebugName() const noexcept
{
    return m_debugName;
}

#pragma endregion
