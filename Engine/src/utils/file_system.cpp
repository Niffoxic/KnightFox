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
#include "engine/utils/file_system.h"
#include "engine/utils/helpers.h"
#include "engine/system/exception/win_exception.h"
#include "engine/utils/logger.h"

#pragma region Impl_Declaration
class KFEFileSystem::Impl
{
public:
	 Impl() = default;
	~Impl() = default;

	NODISCARD bool OpenForRead (_In_ const std::string& path);
	NODISCARD bool OpenForWrite(_In_ const std::string& path);

	void Close();
	NODISCARD bool ReadBytes(_Out_writes_bytes_all_(size) void*  dest,
							_In_                          size_t size) const;
	NODISCARD bool WriteBytes(_In_reads_bytes_(size) const void* data,
							_In_                     size_t      size) const;

	NODISCARD bool ReadUInt32	 (_Out_      std::uint32_t& value ) const;
	NODISCARD bool WriteUInt32	 (_In_       std::uint32_t  value ) const;
	NODISCARD bool ReadString	 (_Out_      std::string&   outStr) const;
	NODISCARD bool WriteString	 (_In_ const std::string&   str   ) const;
	NODISCARD bool WritePlainText(_In_ const std::string&   str   ) const;

	NODISCARD bool          IsOpen	   () const;
	NODISCARD std::uint64_t GetFileSize() const;

private:
	HANDLE m_fileHandle{ INVALID_HANDLE_VALUE };
	bool   m_bReadMode{ false };
};

#pragma endregion

#pragma region FileSystem_Implementation

KFEFileSystem::KFEFileSystem()
	: m_impl(std::make_unique<KFEFileSystem::Impl>())
{}

KFEFileSystem::~KFEFileSystem() = default;

KFEFileSystem::KFEFileSystem(KFEFileSystem&&) = default;
KFEFileSystem& KFEFileSystem::operator=(KFEFileSystem&&) = default;

_Use_decl_annotations_
bool KFEFileSystem::OpenForRead(const std::string& path)
{
	return m_impl->OpenForRead(path);
}

_Use_decl_annotations_
bool KFEFileSystem::OpenForWrite(const std::string& path)
{
	return m_impl->OpenForWrite(path);
}

void KFEFileSystem::Close()
{
	if (!m_impl) return;
	m_impl->Close();
}

_Use_decl_annotations_
bool KFEFileSystem::ReadBytes(void* dest, size_t size) const
{
	return m_impl->ReadBytes(dest, size);
}

_Use_decl_annotations_
bool KFEFileSystem::WriteBytes(const void* data, size_t size) const
{
	return m_impl->WriteBytes(data, size);
}

_Use_decl_annotations_
bool KFEFileSystem::ReadUInt32(uint32_t& value) const
{
	return m_impl->ReadUInt32(value);
}

_Use_decl_annotations_
bool KFEFileSystem::WriteUInt32(uint32_t value) const
{
	return m_impl->WriteUInt32(value);
}

_Use_decl_annotations_
bool KFEFileSystem::ReadString(std::string& outStr) const
{
	return m_impl->ReadString(outStr);
}

_Use_decl_annotations_
bool KFEFileSystem::WriteString(const std::string& str) const
{
	return m_impl->WriteString(str);
}

_Use_decl_annotations_
bool KFEFileSystem::WritePlainText(const std::string& str) const
{
	return m_impl->WritePlainText(str);
}

_Use_decl_annotations_
uint64_t KFEFileSystem::GetFileSize() const
{
	return m_impl->GetFileSize();
}

_Use_decl_annotations_
bool KFEFileSystem::IsOpen() const
{
	return m_impl->IsOpen();
}

#pragma endregion

#pragma region Impl_Implementation
_Use_decl_annotations_
bool KFEFileSystem::Impl::OpenForRead(const std::string & path)
{
	std::wstring w_path = std::wstring(path.begin(), path.end());
	m_fileHandle = CreateFile(
		w_path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	m_bReadMode = true;
	return m_fileHandle != INVALID_HANDLE_VALUE;
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::OpenForWrite(const std::string& path)
{
	auto file = kfe_helpers::SplitPathFile(path);
	if (!kfe_helpers::CreateDirectories(file.DirectoryNames))
	{
#if defined(DEBUG) || defined(_DEBUG)
		THROW_WIN();
#else
		LOG_INFO("Failed To Create Directories at path {}", path);
#endif
	}

	std::wstring w_path = std::wstring(path.begin(), path.end());
	m_fileHandle = CreateFile(
		w_path.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	m_bReadMode = false;
	return m_fileHandle != INVALID_HANDLE_VALUE;
}

void KFEFileSystem::Impl::Close()
{
	if (m_fileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_fileHandle);
		m_fileHandle = INVALID_HANDLE_VALUE;
		m_bReadMode = false;
	}
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::ReadBytes(void* dest, size_t size) const
{
	if (!m_bReadMode || m_fileHandle == INVALID_HANDLE_VALUE) return false;

	DWORD bytesRead = 0;
	return ReadFile(m_fileHandle, dest, static_cast<DWORD>(size), &bytesRead, nullptr) && bytesRead == size;
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::WriteBytes(const void* data, size_t size) const
{
	if (m_bReadMode || m_fileHandle == INVALID_HANDLE_VALUE) return false;

	DWORD bytesWritten = 0;
	return WriteFile(m_fileHandle, data, static_cast<DWORD>(size), &bytesWritten, nullptr) && bytesWritten == size;
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::ReadUInt32(uint32_t& value) const
{
	return ReadBytes(&value, sizeof(uint32_t));
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::WriteUInt32(uint32_t value) const
{
	return WriteBytes(&value, sizeof(uint32_t));
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::ReadString(std::string& outStr) const
{
	uint32_t len;
	if (!ReadUInt32(len)) return false;

	std::string buffer(len, '\0');
	if (!ReadBytes(buffer.data(), len)) return false;

	outStr = std::move(buffer);

	return true;
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::WriteString(const std::string& str) const
{
	uint32_t len = static_cast<uint32_t>(str.size());
	return WriteUInt32(len) && WriteBytes(str.data(), len);
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::WritePlainText(const std::string& str) const
{
	if (m_bReadMode || m_fileHandle == INVALID_HANDLE_VALUE) return false;

	DWORD bytesWritten = 0;
	std::string line = str + "\n";
	return WriteFile(m_fileHandle, line.c_str(), static_cast<DWORD>(line.size()), &bytesWritten, nullptr);
}

_Use_decl_annotations_
uint64_t KFEFileSystem::Impl::GetFileSize() const
{
	if (m_fileHandle == INVALID_HANDLE_VALUE) return 0;

	LARGE_INTEGER size{};
	if (!::GetFileSizeEx(m_fileHandle, &size)) return 0;

	return static_cast<uint64_t>(size.QuadPart);
}

_Use_decl_annotations_
bool KFEFileSystem::Impl::IsOpen() const
{
	return m_fileHandle != INVALID_HANDLE_VALUE;
}

#pragma endregion
