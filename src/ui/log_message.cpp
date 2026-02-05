/**
 * @file log_message.cpp
 * @brief Implementation of LogMessage formatting utilities
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/log_message.h"

#include <iomanip>
#include <sstream>

namespace gimp {

std::string LogMessage::severityString() const
{
    switch (severity) {
        case LogSeverity::Trace:
            return "TRACE";
        case LogSeverity::Debug:
            return "DEBUG";
        case LogSeverity::Info:
            return "INFO";
        case LogSeverity::Warning:
            return "WARN";
        case LogSeverity::Error:
            return "ERROR";
        case LogSeverity::Critical:
            return "CRITICAL";
        case LogSeverity::Off:
            return "OFF";
        default:
            return "UNKNOWN";
    }
}

std::string LogMessage::timeString() const
{
    const auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << localTime.tm_hour << ':' << std::setw(2)
        << localTime.tm_min << ':' << std::setw(2) << localTime.tm_sec;
    return oss.str();
}

std::string LogMessage::formattedLine() const
{
    std::ostringstream oss;
    oss << '[' << timeString() << "] [" << severityString() << "] " << message;
    if (!source.empty()) {
        oss << " (" << source << ')';
    }
    return oss.str();
}

}  // namespace gimp
