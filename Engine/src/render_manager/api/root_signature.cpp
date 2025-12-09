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

#include "engine/utils/logger.h"

#pragma region Impl_Definition

class kfe::KFERootSignature::Impl 
{
public:
	 Impl() = default;
	~Impl();

	NODISCARD bool Initialize(_In_ const KFE_RG_CREATE_DESC& desc) noexcept;

	NODISCARD bool InitializeFromSerialized(
		_In_ KFEDevice*		device,
		_In_ const void*	blob,
		_In_ size_t			blobSize,
		_In_ const wchar_t* debugName = nullptr) noexcept;

	NODISCARD bool InitializeFromFile(
		_In_ KFEDevice*		device,
		_In_ const wchar_t* filePath,
		_In_ const wchar_t* debugName = nullptr) noexcept;

	NODISCARD bool Destroy() noexcept;
			  void Reset  () noexcept;

	NODISCARD		bool  IsInitialized() const noexcept;
	NODISCARD		void* GetNative	   ()		noexcept;
	NODISCARD const void* GetNative    () const noexcept;

	NODISCARD KFE_RG_CREATE_DESC GetCreateDesc() const noexcept;

	void Swap(_In_ KFERootSignature& other) noexcept;

	void						  SetDebugName(_In_opt_ const wchar_t* name) noexcept;
	NODISCARD const std::wstring& GetDebugName() const noexcept;


private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature{ nullptr };
	KFE_RG_CREATE_DESC							m_desc		   {};
	std::wstring								m_debugName;
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
		if (!m_impl->Destroy()) LOG_ERROR("Failed to Destoy");
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
bool kfe::KFERootSignature::Initialize(_In_ const KFE_RG_CREATE_DESC& desc) noexcept
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
	_In_ size_t			blobSize,
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
	if (!Destroy()) LOG_ERROR("Failed to destroy!");
}

_Use_decl_annotations_
bool kfe::KFERootSignature::Impl::Initialize(_In_ const KFE_RG_CREATE_DESC& desc) noexcept
{
	if (!desc.Device)
	{
		return false;
	}

	const auto* rootParams	   = static_cast<const D3D12_ROOT_PARAMETER1*>(desc.RootParameters);
	const auto* staticSamplers = static_cast<const D3D12_STATIC_SAMPLER_DESC*>(desc.StaticSamplers);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc{};
	rsDesc.Version					  = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rsDesc.Desc_1_1.NumParameters	  = desc.NumRootParameters;
	rsDesc.Desc_1_1.pParameters		  = rootParams;
	rsDesc.Desc_1_1.NumStaticSamplers = desc.NumStaticSamplers;
	rsDesc.Desc_1_1.pStaticSamplers   = staticSamplers;
	rsDesc.Desc_1_1.Flags			  = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(desc.Flags);

	Microsoft::WRL::ComPtr<ID3DBlob> serialized;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &serialized, &errors);
	if (FAILED(hr) || !serialized)
	{
		return false;
	}

	ID3D12Device* nativeDevice = desc.Device->GetNative();
	if (!nativeDevice)
	{
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	hr = nativeDevice->CreateRootSignature(
		0,
		serialized->GetBufferPointer(),
		serialized->GetBufferSize(),
		IID_PPV_ARGS(&rootSig));

	if (FAILED(hr))
	{
		return false;
	}

	if (!Destroy()) LOG_ERROR("Failed to destroy!");

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
	_In_ size_t			blobSize,
	_In_ const wchar_t* debugName) noexcept
{
	if (!device || !blob || blobSize == 0)
	{
		return false;
	}

	ID3D12Device* nativeDevice = device->GetNative();
	if (!nativeDevice)
	{
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	HRESULT hr = nativeDevice->CreateRootSignature(
		0,
		blob,
		blobSize,
		IID_PPV_ARGS(&rootSig));

	if (FAILED(hr))
	{
		return false;
	}

	if (!Destroy()) LOG_ERROR("Failed to destroy!");

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
		return false;
	}

	LARGE_INTEGER fileSize{};
	if (!::GetFileSizeEx(hFile, &fileSize))
	{
		::CloseHandle(hFile);
		return false;
	}

	if (fileSize.QuadPart <= 0 || static_cast<ULONGLONG>(fileSize.QuadPart) > static_cast<ULONGLONG>(SIZE_MAX))
	{
		::CloseHandle(hFile);
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
				LOG_ERROR("Failed to null name!");
			}
		}
		return;
	}

	m_debugName = name;
	if (m_rootSignature)
	{
		m_rootSignature->SetName(m_debugName.c_str());
	}
}

_Use_decl_annotations_
const std::wstring& kfe::KFERootSignature::Impl::GetDebugName() const noexcept
{
	return m_debugName;
}

#pragma endregion
