#pragma once
#include <string>
#include <iostream>
#include <iomanip>

#if WIN32
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#endif

#include <g3log/logmessage.hpp>

namespace cgr {

#if WIN32
constexpr int kMutedTextColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
constexpr int kTextColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr int kRedColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
constexpr int kBlueColor = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
#endif

struct StandardOutSink
{
    int min_log_level = g3::kDebugValue; // log everything by default
    std::mutex log_mutex;

    StandardOutSink(int minimum_log_level = g3::kDebugValue)
        : min_log_level(minimum_log_level)
    {}

#if WIN32
    WORD getConsoleLevelColor(const int level) const
    {
        switch (level)
        {
            case g3::kWarningValue:
                return kRedColor;
                break;
            case g3::kDebugValue:
                return kBlueColor;
                break;
            default:
                return kTextColor;
                break;
        }
    }
#endif

    void ReceiveLogMessage(g3::LogMessageMover logEntry)
    {
        auto level = logEntry.get()._level;

        // skip messages that are below the minimum log-level
        if (level.value < min_log_level)
            return;

        std::lock_guard<std::mutex> guard(log_mutex);

        g3::LogMessage entry = logEntry.get();
#ifdef WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, getConsoleLevelColor(entry._level.value));
#endif

        std::cout << std::setw(8) << std::left << entry.level() << " ";

        std::string function = entry.function();

        size_t pos = function.rfind("::");
        if (pos == std::string::npos)
            pos = 0;
        else
        {
            pos = function.rfind("::", pos - 2);
            if (pos == std::string::npos)
                pos = 0;
            else
                pos += 2;
        }
        std::string func = function.substr(pos);

#if WIN32
        SetConsoleTextAttribute(hConsole, kMutedTextColor);
#endif
        std::cout << '[' << func << ": " << entry.line() << "] ";
#if WIN32
        SetConsoleTextAttribute(hConsole, kTextColor);
#endif
        std::cout << entry.message() << std::endl;
    }
};
} // namespace cgr
