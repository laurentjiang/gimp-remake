/**
 * @file log_bridge.h
 * @brief Qt object that bridges spdlog messages to the UI thread
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

#include "log_message.h"
#include "log_sink.h"

#include <QObject>
#include <QTimer>

#include <memory>

namespace gimp {

/**
 * @brief Qt object that bridges spdlog messages to UI thread
 *
 * This class:
 * 1. Owns a QtForwardingSink that receives log messages from any thread.
 * 2. Uses a QTimer to periodically drain the sink's buffer.
 * 3. Emits Qt signals with the drained messages (on the main thread).
 * 4. Can also subscribe to EventBus for LogMessageEvent (optional).
 *
 * UI widgets (LogPanel, ToastNotification) connect to its signals.
 */
class LogBridge : public QObject {
    Q_OBJECT

  public:
    /**
     * @brief Construct a LogBridge
     * @param parent Optional parent QObject
     */
    explicit LogBridge(QObject* parent = nullptr);
    ~LogBridge() override;

    /**
     * @brief Get the sink owned by this bridge
     * @return Raw pointer to the sink (ownership remains with LogBridge)
     *
     * The sink can be registered with spdlog as a global sink.
     */
    QtForwardingSink* sink() { return sink_.get(); }

    /**
     * @brief Start the timer that drains messages
     * @param intervalMs Timer interval in milliseconds (default: 100)
     */
    void start(int intervalMs = 100);

    /**
     * @brief Stop the timer
     */
    void stop();

    /**
     * @brief Manually trigger a drain (for testing or immediate processing)
     */
    void drainNow();

  signals:
    /**
     * @brief Emitted when one or more log messages are ready for UI display
     * @param messages Vector of log messages
     *
     * This signal is emitted on the main thread. Connected slots can safely
     * update Qt widgets.
     */
    void logMessagesReady(const std::vector<LogMessage>& messages);

    /**
     * @brief Emitted when a single log message arrives (convenience signal)
     * @param message The log message
     */
    void logMessageReady(const LogMessage& message);

  private slots:
    void onTimer();

  private:
    std::unique_ptr<QtForwardingSink> sink_;
    QTimer timer_;
};

}  // namespace gimp
