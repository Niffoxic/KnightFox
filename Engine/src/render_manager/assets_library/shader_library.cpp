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
#include "engine/render_manager/assets_library/shader_library.h"
#include "engine/utils/file_system.h"
#include "engine/utils/json_loader.h"

#include <unordered_map>
#include <mutex>
#include <sstream>
#include <d3d12.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

#include "engine/utils/logger.h"
#include "engine/utils/helpers.h"

using Microsoft::WRL::ComPtr;

namespace
{
	constexpr const char* kDefaultManifestName = "shader_cache.json";

	inline std::string NormalizePath(std::string path)
	{
		for (char& c : path)
		{
			if (c == '\\')
			{
				c = '/';
			}
		}
		return path;
	}

	inline std::string GetFileExtensionLower(const std::string& path)
	{
		const std::string normalized = NormalizePath(path);
		const auto        pos = normalized.find_last_of('.');
		if (pos == std::string::npos)
		{
			return {};
		}

		std::string ext = normalized.substr(pos + 1);
		return kfe_helpers::ToLowerAscii(ext);
	}

	inline std::string JoinPath(const std::string& dir, const std::string& file)
	{
		if (dir.empty())
		{
			return file;
		}

		std::string result = dir;
		if (!result.empty())
		{
			const char last = result.back();
			if (last != '/' && last != '\\')
			{
				result.push_back('/');
			}
		}
		result += file;
		return result;
	}

	inline std::string MakeFileNameFromKey(const std::string& key)
	{
		std::hash<std::string> hasher;
		const std::size_t      h = hasher(key);

		std::ostringstream oss;
		oss << std::hex << h;
		oss << ".cso";
		return oss.str();
	}

	inline std::string MakeCacheKey(const kfe::KFEShaderLibrary::COMPILE_DESC& desc)
	{
		if (!desc.CacheKeyOverride.empty())
		{
			return desc.CacheKeyOverride;
		}

		std::ostringstream oss;
		oss << desc.SourcePath << "|" << desc.EntryPoint << "|" << desc.TargetProfile;
		return oss.str();
	}

	inline UINT BuildD3DCompileFlags(const kfe::KFEShaderLibrary::COMPILE_DESC& desc)
	{
		UINT flags = 0u;

		if (desc.EnableDebug)
		{
			flags |= D3DCOMPILE_DEBUG;
			flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
		}
		if (desc.WarningsAsErrors)
		{
			flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
		}

		flags |= desc.ReservedFlags;
		return flags;
	}

	inline bool EnsureDirectoryForFile(const std::string& filePath)
	{
		std::string normalized = NormalizePath(filePath);
		const auto  pos = normalized.find_last_of('/');
		if (pos == std::string::npos)
		{
			return true;
		}

		std::string directory = normalized.substr(0, pos + 1);
		if (directory.empty())
		{
			return true;
		}

		return kfe_helpers::CreateDirectories(directory);
	}

	inline bool LoadBlobFromFile(const std::string& path, ComPtr<ID3DBlob>& blobOut)
	{
		KFEFileSystem fs;
		if (!fs.OpenForRead(path))
		{
			LOG_WARNING("KFEShaderLibrary: Failed to open shader binary for read: {}", path);
			return false;
		}

		const auto size = fs.GetFileSize();
		if (size == 0)
		{
			LOG_WARNING("KFEShaderLibrary: Shader binary is empty: {}", path);
			fs.Close();
			return false;
		}

		ComPtr<ID3DBlob> blob;
		if (FAILED(D3DCreateBlob(static_cast<SIZE_T>(size), blob.GetAddressOf())))
		{
			LOG_ERROR("KFEShaderLibrary: Failed to allocate blob for shader binary: {}", path);
			fs.Close();
			return false;
		}

		if (!fs.ReadBytes(blob->GetBufferPointer(), static_cast<size_t>(size)))
		{
			LOG_ERROR("KFEShaderLibrary: Failed to read shader binary data: {}", path);
			fs.Close();
			return false;
		}

		fs.Close();
		blobOut = blob;
		return true;
	}

	inline bool SaveBlobToFile(const std::string& path, ID3DBlob* blob)
	{
		if (!blob)
		{
			return false;
		}

		if (!EnsureDirectoryForFile(path))
		{
			LOG_ERROR("KFEShaderLibrary: Failed to create directories for shader binary: {}", path);
			return false;
		}

		KFEFileSystem fs;
		if (!fs.OpenForWrite(path))
		{
			LOG_ERROR("KFEShaderLibrary: Failed to open shader binary for write: {}", path);
			return false;
		}

		const void* data = blob->GetBufferPointer();
		const size_t size = blob->GetBufferSize();

		if (!fs.WriteBytes(data, size))
		{
			LOG_ERROR("KFEShaderLibrary: Failed to write shader binary: {}", path);
			fs.Close();
			return false;
		}

		fs.Close();
		return true;
	}

	inline void LogCompileError(ID3DBlob* errorBlob, const std::string& sourcePath)
	{
		if (!errorBlob)
		{
			LOG_ERROR("KFEShaderLibrary: Shader compilation failed for {} with unknown error.", sourcePath);
			return;
		}

		const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
		const size_t msgLen = errorBlob->GetBufferSize();

		std::string errorStr(msg, msg + msgLen);
		LOG_ERROR("KFEShaderLibrary: Shader compilation error for {}:\n{}", sourcePath, errorStr);
	}
} // namespace

#pragma region Impl_Definition

class kfe::KFEShaderLibrary::Impl
{
public:
	struct ShaderRecord
	{
		ComPtr<ID3DBlob> Blob;
		COMPILE_DESC     Desc;
		std::string      BinaryPath;
	};

	 Impl() = default;
	~Impl() = default;

	void SetCacheDirectory(_In_ const std::string& directory);
	NODISCARD const std::string& GetCacheDirectory() const noexcept;

	NODISCARD ID3DBlob* GetShader(_In_ const std::string& path);
	NODISCARD ID3DBlob* GetShader(_In_ const COMPILE_DESC& desc);

	NODISCARD bool Contains(_In_ const std::string& cacheKey) const;
	NODISCARD bool Remove(_In_ const std::string& cacheKey);

	void                 Clear() noexcept;
	NODISCARD std::size_t GetCachedShaderCount() const noexcept;

	NODISCARD bool Load(_In_ const std::string& path);
	NODISCARD bool Save(_In_ const std::string& path) const;
	NODISCARD bool ReloadFromDisk();

private:
	ID3DBlob* GetShaderInternal(_In_ const COMPILE_DESC& desc);
	bool      LoadManifest(_In_ const std::string& path);
	bool      SaveManifest(_In_ const std::string& path) const;

private:
	std::string m_cacheDirectory;
	std::string m_manifestPath;

	std::unordered_map<std::string, ShaderRecord> m_cache;
	mutable std::mutex                            m_mutex;
};

#pragma endregion

#pragma region Shader_Library_Body

kfe::KFEShaderLibrary::KFEShaderLibrary()
	: m_impl(std::make_unique<Impl>())
{}

kfe::KFEShaderLibrary::~KFEShaderLibrary() = default;

void kfe::KFEShaderLibrary::SetCacheDirectory(_In_ const std::string& directory)
{
	m_impl->SetCacheDirectory(directory);
}

_Use_decl_annotations_
const std::string& kfe::KFEShaderLibrary::GetCacheDirectory() const noexcept
{
	return m_impl->GetCacheDirectory();
}

_Use_decl_annotations_
ID3DBlob* kfe::KFEShaderLibrary::GetShader(_In_ const std::string& path)
{
	return m_impl->GetShader(path);
}

_Use_decl_annotations_
ID3DBlob* kfe::KFEShaderLibrary::GetShader(_In_ const COMPILE_DESC& desc)
{
	return m_impl->GetShader(desc);
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Contains(_In_ const std::string& cacheKey) const
{
	return m_impl->Contains(cacheKey);
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Remove(_In_ const std::string& cacheKey)
{
	return m_impl->Remove(cacheKey);
}

void kfe::KFEShaderLibrary::Clear() noexcept
{
	m_impl->Clear();
}

_Use_decl_annotations_
std::size_t kfe::KFEShaderLibrary::GetCachedShaderCount() const noexcept
{
	return m_impl->GetCachedShaderCount();
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Load(_In_ const std::string& path)
{
	return m_impl->Load(path);
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Save(_In_ const std::string& path) const
{
	return m_impl->Save(path);
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::ReloadFromDisk()
{
	return m_impl->ReloadFromDisk();
}

#pragma endregion

#pragma region Impl_Body

_Use_decl_annotations_
void kfe::KFEShaderLibrary::Impl::SetCacheDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_cacheDirectory = NormalizePath(directory);

	if (!m_cacheDirectory.empty())
	{
		if (!kfe_helpers::CreateDirectories(m_cacheDirectory))
		{
			LOG_WARNING("KFEShaderLibrary::SetCacheDirectory: Failed to create cache directory: {}", m_cacheDirectory);
		}
	}
}

_Use_decl_annotations_
const std::string& kfe::KFEShaderLibrary::Impl::GetCacheDirectory() const noexcept
{
	return m_cacheDirectory;
}

_Use_decl_annotations_
ID3DBlob* kfe::KFEShaderLibrary::Impl::GetShader(const std::string& path)
{
	const std::string ext = GetFileExtensionLower(path);

	if (ext == "cso")
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const std::string cacheKey = NormalizePath(path);
		auto              it = m_cache.find(cacheKey);
		if (it != m_cache.end() && it->second.Blob)
		{
			return it->second.Blob.Get();
		}

		ShaderRecord rec{};
		rec.Desc.SourcePath = path;
		rec.Desc.EntryPoint = "";
		rec.Desc.TargetProfile = "";
		rec.BinaryPath = path;

		if (!LoadBlobFromFile(path, rec.Blob))
		{
			LOG_ERROR("KFEShaderLibrary: Failed to load precompiled shader: {}", path);
			return nullptr;
		}

		m_cache.emplace(cacheKey, std::move(rec));
		return m_cache[cacheKey].Blob.Get();
	}
	else
	{
		COMPILE_DESC desc{};
		desc.SourcePath = path;
		desc.EntryPoint = "main";
		desc.TargetProfile = "vs_5_0";
		return GetShader(desc);
	}
}

_Use_decl_annotations_
ID3DBlob* kfe::KFEShaderLibrary::Impl::GetShader(const COMPILE_DESC& desc)
{
	return GetShaderInternal(desc);
}

_Use_decl_annotations_
ID3DBlob* kfe::KFEShaderLibrary::Impl::GetShaderInternal(const COMPILE_DESC& desc)
{
	if (desc.SourcePath.empty())
	{
		LOG_ERROR("KFEShaderLibrary::GetShader: SourcePath is empty.");
		return nullptr;
	}

	if (desc.EntryPoint.empty() || desc.TargetProfile.empty())
	{
		LOG_ERROR("KFEShaderLibrary::GetShader: EntryPoint or TargetProfile is empty for {}.", desc.SourcePath);
		return nullptr;
	}

	const std::string cacheKey = MakeCacheKey(desc);

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (!desc.ForceRecompile)
		{
			auto it = m_cache.find(cacheKey);
			if (it != m_cache.end() && it->second.Blob)
			{
				return it->second.Blob.Get();
			}

			if (!m_cacheDirectory.empty())
			{
				const std::string fileName = MakeFileNameFromKey(cacheKey);
				const std::string binaryPath = JoinPath(m_cacheDirectory, fileName);

				ShaderRecord rec{};
				rec.Desc = desc;
				rec.BinaryPath = binaryPath;

				if (LoadBlobFromFile(binaryPath, rec.Blob))
				{
					m_cache.emplace(cacheKey, std::move(rec));
					return m_cache[cacheKey].Blob.Get();
				}
			}
		}
	}

	const std::wstring widePath = kfe_helpers::AnsiToWide(desc.SourcePath);
	const UINT         flags = BuildD3DCompileFlags(desc);

	ComPtr<ID3DBlob> shaderBlob;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3DCompileFromFile(
		widePath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		desc.EntryPoint.c_str(),
		desc.TargetProfile.c_str(),
		flags,
		0,
		shaderBlob.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (FAILED(hr))
	{
		LogCompileError(errorBlob.Get(), desc.SourcePath);
		return nullptr;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		ShaderRecord rec{};
		rec.Blob = shaderBlob;
		rec.Desc = desc;

		if (!m_cacheDirectory.empty())
		{
			const std::string fileName = MakeFileNameFromKey(cacheKey);
			const std::string binaryPath = JoinPath(m_cacheDirectory, fileName);
			if (!SaveBlobToFile(binaryPath, shaderBlob.Get()))
			{
				LOG_WARNING("KFEShaderLibrary: Compiled shader but failed to save binary: {}", binaryPath);
			}
			rec.BinaryPath = binaryPath;
		}

		m_cache[cacheKey] = std::move(rec);

		LOG_INFO("KFEShaderLibrary: Compiled and cached shader: {} (Entry: {}, Target: {})",
			desc.SourcePath, desc.EntryPoint, desc.TargetProfile);

		return m_cache[cacheKey].Blob.Get();
	}
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::Contains(_In_ const std::string& cacheKey) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_cache.find(cacheKey) != m_cache.end();
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::Remove(const std::string& cacheKey)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	const auto                   it = m_cache.find(cacheKey);
	if (it == m_cache.end())
	{
		return false;
	}

	m_cache.erase(it);
	return true;
}

void kfe::KFEShaderLibrary::Impl::Clear() noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_cache.clear();
}

_Use_decl_annotations_
std::size_t kfe::KFEShaderLibrary::Impl::GetCachedShaderCount() const noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_cache.size();
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::Load(const std::string& path)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_cache.clear();

	if (!LoadManifest(path))
	{
		LOG_ERROR("KFEShaderLibrary::Load: Failed to load manifest: {}", path);
		return false;
	}

	m_manifestPath = path;
	return true;
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::Save(const std::string& path) const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!SaveManifest(path))
	{
		LOG_ERROR("KFEShaderLibrary::Save: Failed to save manifest: {}", path);
		return false;
	}

	return true;
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::ReloadFromDisk()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_manifestPath.empty())
	{
		LOG_WARNING("KFEShaderLibrary::ReloadFromDisk: No manifest path set. Call Load() first.");
		return false;
	}

	m_cache.clear();
	return LoadManifest(m_manifestPath);
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::LoadManifest(const std::string& path)
{
	JsonLoader root;
	root.Load(path);

	if (!root.IsValid())
	{
		LOG_WARNING("KFEShaderLibrary::LoadManifest: Json manifest is invalid: {}", path);
		return false;
	}

	if (root.Contains("CacheDirectory"))
	{
		const JsonLoader& cdNode = root["CacheDirectory"];
		const std::string cd = cdNode.GetValue();
		if (!cd.empty())
		{
			m_cacheDirectory = NormalizePath(cd);
		}
	}

	if (!root.Contains("Shaders"))
	{
		return true;
	}

	const JsonLoader& shadersNode = root["Shaders"];
	if (!shadersNode.IsObject())
	{
		return true;
	}

	for (const auto& kv : shadersNode)
	{
		const std::string& cacheKey = kv.first;
		const JsonLoader& shaderJson = kv.second;

		ShaderRecord rec{};
		rec.Desc.SourcePath = shaderJson.Contains("SourcePath") ? shaderJson["SourcePath"].GetValue() : "";
		rec.Desc.EntryPoint = shaderJson.Contains("EntryPoint") ? shaderJson["EntryPoint"].GetValue() : "";
		rec.Desc.TargetProfile = shaderJson.Contains("TargetProfile") ? shaderJson["TargetProfile"].GetValue() : "";
		rec.Desc.CacheKeyOverride = shaderJson.Contains("CacheKeyOverride") ? shaderJson["CacheKeyOverride"].GetValue() : "";
		rec.Desc.ForceRecompile = false;

		if (shaderJson.Contains("EnableDebug"))
		{
			rec.Desc.EnableDebug = shaderJson["EnableDebug"].AsBool(false);
		}
		if (shaderJson.Contains("WarningsAsErrors"))
		{
			rec.Desc.WarningsAsErrors = shaderJson["WarningsAsErrors"].AsBool(true);
		}
		if (shaderJson.Contains("ReservedFlags"))
		{
			rec.Desc.ReservedFlags = static_cast<std::uint32_t>(shaderJson["ReservedFlags"].AsInt(0));
		}

		if (shaderJson.Contains("BinaryPath"))
		{
			rec.BinaryPath = shaderJson["BinaryPath"].GetValue();
		}
		else if (!m_cacheDirectory.empty())
		{
			const std::string fileName = MakeFileNameFromKey(cacheKey);
			const std::string binaryPath = JoinPath(m_cacheDirectory, fileName);
			rec.BinaryPath = binaryPath;
		}

		if (rec.BinaryPath.empty())
		{
			LOG_WARNING("KFEShaderLibrary::LoadManifest: Shader {} has no BinaryPath. Skipping.", cacheKey);
			continue;
		}

		ComPtr<ID3DBlob> blob;
		if (!LoadBlobFromFile(rec.BinaryPath, blob))
		{
			LOG_WARNING("KFEShaderLibrary::LoadManifest: Failed to load shader binary {} for key {}.",
				rec.BinaryPath, cacheKey);
			continue;
		}

		rec.Blob = blob;
		m_cache.emplace(cacheKey, std::move(rec));
	}

	LOG_INFO("KFEShaderLibrary::LoadManifest: Loaded {} shaders from {}", m_cache.size(), path);
	return true;
}

_Use_decl_annotations_
bool kfe::KFEShaderLibrary::Impl::SaveManifest(const std::string& path) const
{
	if (!EnsureDirectoryForFile(path))
	{
		LOG_ERROR("KFEShaderLibrary::SaveManifest: Failed to create directories for manifest: {}", path);
		return false;
	}

	JsonLoader root;

	// Cache directory
	if (!m_cacheDirectory.empty())
	{
		root["CacheDirectory"] = m_cacheDirectory;
	}

	JsonLoader& shadersNode = root["Shaders"];

	for (const auto& kv : m_cache)
	{
		const std::string& cacheKey = kv.first;
		const ShaderRecord& rec = kv.second;

		JsonLoader& shaderJson = shadersNode[cacheKey];

		shaderJson["SourcePath"] = rec.Desc.SourcePath;
		shaderJson["EntryPoint"] = rec.Desc.EntryPoint;
		shaderJson["TargetProfile"] = rec.Desc.TargetProfile;
		shaderJson["BinaryPath"] = rec.BinaryPath;
		shaderJson["CacheKeyOverride"] = rec.Desc.CacheKeyOverride;

		shaderJson["EnableDebug"] = rec.Desc.EnableDebug ? "true" : "false";
		shaderJson["WarningsAsErrors"] = rec.Desc.WarningsAsErrors ? "true" : "false";

		{
			std::ostringstream oss;
			oss << rec.Desc.ReservedFlags;
			shaderJson["ReservedFlags"] = oss.str();
		}
	}

	root.Save(path);

	LOG_INFO("KFEShaderLibrary::SaveManifest: Saved {} shaders to {}", m_cache.size(), path);
	return true;
}
#pragma endregion
