/**
 * @file log_sink.h
 * @brief Thread-safe spdlog sink that forwards messages to Qt UI
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

#include "log_message.h"

#include <spdlog/sinks/base_sink.h>

#include <mutex>
#include <vector>

namespace gimp {

/**
 * @brief Thread-safe spdlog sink that queues messages for UI display
 *
 * This sink implements spdlog's base_sink with a mutex. When a log message arrives
 * (from any thread), it is converted to a LogMessage and added to an internal buffer.
 * The buffer can be drained by the main thread via drain().
 *
 * The sink is designed to be owned by LogBridge, which periodically calls drain()
 * and forwards the messages to Qt signals.
 */
class QtForwardingSink : public spdlog::sinks::base_sink<std::mutex> {
  public:
    QtForwardingSink();
    ~QtForwardingSink() override = default;

    /**
     * @brief Remove all queued messages and return them
     * @return Vector of LogMessage objects that were queued
     *
     * This method should only be called from the main (Qt) thread.
     * It is thread-safe with respect to the sink's internal mutex.
     */
    std::vector<LogMessage> drain();

    /**
     * @brief Get current number of queued messages
     * @return Count of messages waiting in the buffer
     */
    std::size_t queuedCount() const;

    /**
     * @brief Set maximum buffer size (oldest messages are dropped when exceeded)
     * @param max Maximum number of messages to keep
     */
    void setMaxBufferSize(std::size_t max);

  protected:
    /**
     * @brief spdlog sink implementation: process a log message
     * @param msg The spdlog log message
     */
    void sink_it_(const spdlog::details::log_msg& msg) override;

    /**
     * @brief spdlog sink implementation: flush (no-op for us)
     */
    void flush_() override {}

  private:
    std::vector<LogMessage> m_buffer;
    mutable std::mutex m_bufferMutex;
    std::size_t m_maxBufferSize = 1000;  ///< Maximum messages to keep before dropping oldest
};

}  // namespace gimp
