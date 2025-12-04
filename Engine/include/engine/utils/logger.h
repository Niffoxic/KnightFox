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

#include <string>
#include <string_view>
#include <format>
#include <utility>

#include "engine/utils/file_system.h"

typedef struct KFE_LOGGER_CREATE_DESC
{
    _In_ std::string LogPath;
    _In_ std::string LogPrefix;
    _In_ bool        EnableTerminal;
} KFE_LOGGER_CREATE_DESC;

class KFE_API KFELogger
{
public:
    KFELogger(_In_reads_(1) const KFE_LOGGER_CREATE_DESC* desc);
    ~KFELogger();

    KFELogger(const KFELogger&) = delete;
    KFELogger(KFELogger&&) = delete;

    KFELogger& operator=(const KFELogger&) = delete;
    KFELogger& operator=(KFELogger&&) = delete;

    template<typename... Args>
    bool Info(_In_ std::string_view fmt, Args&&... args)
    {
        return Log(
            "INFO",
            std::vformat(fmt, std::make_format_args(args...)),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            nullptr,
            -1,
            nullptr
        );
    }

    template<typename... Args>
    bool Print(_In_ std::string_view fmt, Args&&... args)
    {
        return Log(
            "PRINT",
            std::vformat(fmt, std::make_format_args(args...)),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            nullptr,
            -1,
            nullptr
        );
    }

    template<typename... Args>
    bool Warning(_In_ std::string_view fmt, Args&&... args)
    {
        return Log(
            "WARN",
            std::vformat(fmt, std::make_format_args(args...)),
            FOREGROUND_RED | FOREGROUND_GREEN,
            nullptr,
            -1,
            nullptr
        );
    }

    template<typename... Args>
    bool Success(_In_ std::string_view fmt, Args&&... args)
    {
        return Log(
            "OK",
            std::vformat(fmt, std::make_format_args(args...)),
            FOREGROUND_GREEN,
            nullptr,
            -1,
            nullptr
        );
    }

    template<typename... Args>
    bool Error(
        _In_z_ const char* file,
        _In_   int              line,
        _In_z_ const char* func,
        _In_   std::string_view fmt,
        Args&&...               args
    )
    {
        return Log(
            "ERROR",
            std::vformat(fmt, std::make_format_args(args...)),
            FOREGROUND_RED | FOREGROUND_INTENSITY,
            file,
            line,
            func
        );
    }

    template<typename... Args>
    bool Fail(
        _In_z_ const char* file,
        _In_   int              line,
        _In_z_ const char* func,
        _In_   std::string_view fmt,
        Args&&...               args
    )
    {
        return Log(
            "FATAL",
            std::vformat(fmt, std::make_format_args(args...)),
            FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED,
            file,
            line,
            func
        );
    }

    _NODISCARD _Check_return_
        std::string GetTimestamp();
    void Close();
private:

    void EnableTerminal();

    _Check_return_
        bool Log(
            _In_     const std::string& prefix,
            _In_     const std::string& message,
            _In_     WORD               color,
            _In_opt_ const char* file = nullptr,
            _In_     int                line = -1,
            _In_opt_ const char* func = nullptr
        );

private:
    KFE_LOGGER_CREATE_DESC m_loggerDesc{};
    HANDLE                 m_consoleHandle{};
    HANDLE                 m_mutexHandle{};
    KFEFileSystem          m_fileSystem{};
};

extern KFE_API KFELogger* gLogger;

#define INIT_GLOBAL_LOGGER(desc) \
    do { gLogger = new KFELogger(desc); } while(0)

#define LOG_INFO(...)    (gLogger ? gLogger->Info   (__VA_ARGS__) : false)
#define LOG_PRINT(...)   (gLogger ? gLogger->Print  (__VA_ARGS__) : false)
#define LOG_WARNING(...) (gLogger ? gLogger->Warning(__VA_ARGS__) : false)
#define LOG_SUCCESS(...) (gLogger ? gLogger->Success(__VA_ARGS__) : false)

#define LOG_ERROR(...) \
    (gLogger ? gLogger->Error(__FILE__, __LINE__, __func__, __VA_ARGS__) : false)

#define LOG_FAIL(...) \
    (gLogger ? gLogger->Fail(__FILE__, __LINE__, __func__, __VA_ARGS__) : false)
