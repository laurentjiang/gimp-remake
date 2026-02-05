/**
 * @file log_bridge.cpp
 * @brief Implementation of LogBridge
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/log_bridge.h"

#include <spdlog/spdlog.h>

namespace gimp {

LogBridge::LogBridge(QObject* parent) : QObject(parent), sink_(std::make_unique<QtForwardingSink>())
{
    connect(&timer_, &QTimer::timeout, this, &LogBridge::onTimer);
}

LogBridge::~LogBridge()
{
    stop();
}

void LogBridge::start(int intervalMs)
{
    if (!timer_.isActive()) {
        timer_.start(intervalMs);
    }
}

void LogBridge::stop()
{
    if (timer_.isActive()) {
        timer_.stop();
    }
}

void LogBridge::drainNow()
{
    onTimer();
}

void LogBridge::onTimer()
{
    auto messages = sink_->drain();
    if (messages.empty()) {
        return;
    }

    // Emit batch signal
    emit logMessagesReady(messages);

    // Also emit individual signals for convenience
    for (const auto& msg : messages) {
        emit logMessageReady(msg);
    }
}

}  // namespace gimp
