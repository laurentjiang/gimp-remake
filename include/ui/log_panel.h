/**
 * @file log_panel.h
 * @brief Dockable panel displaying application logs with filtering
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

#include "core/event_bus.h"
#include "ui/log_message.h"

#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>
#include <vector>

namespace gimp {

class LogBridge;

/**
 * @brief Dockable panel displaying application logs
 *
 * Features:
 * - List of log entries with severity icons and timestamps
 * - Filter dropdown (All / Warnings+ / Errors only)
 * - Clear button to remove all entries
 * - Copy button to copy selected entries (or all) to clipboard
 * - Maximum entry limit with oldest-first removal
 * - Connects to LogBridge signals for real‑time updates
 * - Can also subscribe to LogMessageEvent via EventBus
 */
class LogPanel : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Construct a LogPanel widget
     * @param parent Optional parent widget
     */
    explicit LogPanel(QWidget* parent = nullptr);
    ~LogPanel() override;

    /**
     * @brief Connect to a LogBridge for receiving messages
     * @param bridge The LogBridge instance (can be nullptr to disconnect)
     *
     * If bridge is not null, LogPanel will connect to its signals and
     * start displaying messages. If null, existing connections are removed.
     */
    void connectToBridge(LogBridge* bridge);

    /**
     * @brief Add a log message to the panel
     * @param message The log message to add
     *
     * This method is thread‑safe because it is called from a Qt signal
     * that is delivered on the main thread.
     */
    void addLogMessage(const LogMessage& message);

    /**
     * @brief Add multiple log messages at once
     * @param messages Vector of log messages
     */
    void addLogMessages(const std::vector<LogMessage>& messages);

    /**
     * @brief Clear all log entries from the panel
     */
    void clear();

    /**
     * @brief Set maximum number of entries to keep
     * @param max Maximum entries (0 = unlimited)
     *
     * When the limit is exceeded, oldest entries are removed.
     */
    void setMaxEntries(std::size_t max);

    /**
     * @brief Get current number of entries
     */
    std::size_t entryCount() const;

  signals:
    /**
     * @brief Emitted when the user requests to copy entries to clipboard
     * @param text The text that was copied
     */
    void entriesCopied(const QString& text);

  private slots:
    void onFilterChanged(int index);
    void onClearClicked();
    void onCopyClicked();
    void onLogMessageReady(const LogMessage& message);
    void onLogMessagesReady(const std::vector<LogMessage>& messages);

  private:
    void setupUi();
    void refreshVisibleItems();
    bool shouldShowMessage(const LogMessage& msg) const;
    QListWidgetItem* createItemForMessage(const LogMessage& msg);
    QString severityIconName(LogSeverity severity) const;
    QString severityColor(LogSeverity severity) const;

    QVBoxLayout* mainLayout_ = nullptr;
    QHBoxLayout* buttonLayout_ = nullptr;
    QComboBox* filterCombo_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QPushButton* copyButton_ = nullptr;
    QListWidget* logList_ = nullptr;

    std::vector<LogMessage> allMessages_;  ///< All messages (including filtered‑out)
    std::size_t maxEntries_ = 1000;        ///< Maximum entries to keep

    enum FilterLevel {
        FilterAll,
        FilterWarningsAndAbove,
        FilterErrorsOnly
    };
    FilterLevel currentFilter_ = FilterAll;

    EventBus::SubscriptionId logEventSub_ = 0;
};

}  // namespace gimp
