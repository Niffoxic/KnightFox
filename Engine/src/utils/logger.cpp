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
#include "engine/utils/logger.h"

#include <iostream>
#include <chrono>

KFELogger* gLogger = nullptr;

_Use_decl_annotations_
KFELogger::KFELogger(const KFE_LOGGER_CREATE_DESC* desc)
{
    if (desc->EnableTerminal) EnableTerminal();

    m_loggerDesc.LogPrefix = desc->LogPrefix;
    m_loggerDesc.EnableTerminal = desc->EnableTerminal;
    m_loggerDesc.LogPath = desc->LogPath;

    m_mutexHandle = CreateMutex(nullptr,
        FALSE,
        nullptr);

    if (!m_fileSystem.OpenForWrite(GetTimestamp())) 
    {
        // Handle Error
    }
}

KFELogger::~KFELogger()
{
    Close();
}

void KFELogger::EnableTerminal()
{
    if (!AllocConsole()) return;

    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);

    m_consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}

_Use_decl_annotations_
bool KFELogger::Log(const std::string& prefix, const std::string& message, WORD color,
    const char* file, int line, const char* func)
{
    std::ostringstream oss;

    std::time_t now = std::time(nullptr);
    std::tm localTime{};
    localtime_s(&localTime, &now);
    oss << "[" << std::put_time(&localTime, "%H:%M:%S") << "] ";

    oss << "[" << prefix << "] ";
    if (file && func && line >= 0)
    {
        oss << "(" << file << ":" << line << " | " << func << ") ";
    }

    oss << "- " << message << "\n";

    std::string refinedMessage = oss.str();
    bool saved = false;

    DWORD res = WaitForSingleObject(m_mutexHandle, 2000);

    if (res == WAIT_OBJECT_0)
    {
        SetConsoleTextAttribute(m_consoleHandle, color);
        std::cout << refinedMessage;
        SetConsoleTextAttribute(m_consoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        saved = m_fileSystem.WritePlainText(refinedMessage);

        ReleaseMutex(m_mutexHandle);
    }
    else if (res == WAIT_TIMEOUT)
    {
        m_fileSystem.WritePlainText("[FAILURE] Logger timeout.\n");
        return false;
    }
    else
    {
        return false;
    }
    return saved;
}

_Use_decl_annotations_
std::string KFELogger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;

    localtime_s(&localTime, &timeNow);


    std::string folder = m_loggerDesc.LogPath;
    if (*folder.rbegin() != '/')
    {
        folder += "/";
    }

    std::string file = m_loggerDesc.LogPrefix;
    if (*file.rbegin() != '_')
    {
        file += "_";
    }

    std::ostringstream oss;
    oss << folder
        << file
        << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S")
        << ".txt";

    return oss.str();
}

void KFELogger::Close()
{
    m_fileSystem.Close();
}
