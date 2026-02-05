/**
 * @file log_sink.cpp
 * @brief Implementation of QtForwardingSink
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/log_sink.h"

#include <spdlog/details/log_msg.h>

#include <cstddef>

namespace gimp {

QtForwardingSink::QtForwardingSink()
{
    // Reserve some initial capacity
    m_buffer.reserve(100);
}

std::vector<LogMessage> QtForwardingSink::drain()
{
    std::vector<LogMessage> messages;
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        messages.swap(m_buffer);
        m_buffer.clear();
        m_buffer.reserve(100);  // Keep some capacity for future messages
    }
    return messages;
}

std::size_t QtForwardingSink::queuedCount() const
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_buffer.size();
}

void QtForwardingSink::setMaxBufferSize(std::size_t max)
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_maxBufferSize = max;
    // If buffer exceeds new max, trim from front (oldest)
    if (m_buffer.size() > m_maxBufferSize) {
        m_buffer.erase(
            m_buffer.begin(),
            m_buffer.begin() + static_cast<std::ptrdiff_t>(m_buffer.size() - m_maxBufferSize));
    }
}

void QtForwardingSink::sink_it_(const spdlog::details::log_msg& msg)
{
    // Convert spdlog level to our severity
    LogSeverity severity = LogSeverity::Info;
    switch (msg.level) {
        case spdlog::level::trace:
            severity = LogSeverity::Trace;
            break;
        case spdlog::level::debug:
            severity = LogSeverity::Debug;
            break;
        case spdlog::level::info:
            severity = LogSeverity::Info;
            break;
        case spdlog::level::warn:
            severity = LogSeverity::Warning;
            break;
        case spdlog::level::err:
            severity = LogSeverity::Error;
            break;
        case spdlog::level::critical:
            severity = LogSeverity::Critical;
            break;
        case spdlog::level::off:
            severity = LogSeverity::Off;
            break;
        default:
            severity = LogSeverity::Info;
            break;
    }

    // Extract message string
    std::string message(msg.payload.data(), msg.payload.size());

    // Extract source (logger name)
    std::string source(msg.logger_name.data(), msg.logger_name.size());

    // Create log message
    LogMessage logMsg;
    logMsg.severity = severity;
    logMsg.timestamp = msg.time;
    logMsg.message = std::move(message);
    logMsg.source = std::move(source);

    // Add to buffer with thread safety
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffer.push_back(std::move(logMsg));

        // Enforce maximum buffer size
        if (m_buffer.size() > m_maxBufferSize) {
            m_buffer.erase(m_buffer.begin());
        }
    }
}

}  // namespace gimp
