/**
 * @file log_message.h
 * @brief Log message data structure for UI error reporting
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

#include <chrono>
#include <string>

namespace gimp {

/**
 * @brief Severity levels for log messages
 *
 * Matches spdlog's severity levels for consistency.
 */
enum class LogSeverity {
    Trace,     ///< Detailed tracing information
    Debug,     ///< Debug-level information
    Info,      ///< Informational messages
    Warning,   ///< Warning conditions
    Error,     ///< Error conditions
    Critical,  ///< Critical errors
    Off        ///< No logging
};

/**
 * @brief Single log message entry
 *
 * Contains all metadata needed to display a log entry in the UI.
 */
struct LogMessage {
    LogSeverity severity = LogSeverity::Info;            ///< Severity level of the log message.
    std::chrono::system_clock::time_point timestamp;     ///< When the message was created.
    std::string message;                                 ///< The log message text.
    std::string source;                                  ///< Component that generated the message (e.g., "render", "io", "tool")

    /**
     * @brief Convert severity to human-readable string
     * @return Short string representation (e.g., "INFO", "WARN")
     */
    std::string severityString() const;

    /**
     * @brief Format timestamp as HH:MM:SS
     * @return Time string in local time
     */
    std::string timeString() const;

    /**
     * @brief Create a formatted display line for the log panel
     * @return Formatted string: "[HH:MM:SS] [SEVERITY] message"
     */
    std::string formattedLine() const;
};

}  // namespace gimp
