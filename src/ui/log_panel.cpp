/**
 * @file log_panel.cpp
 * @brief Implementation of LogPanel
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/log_panel.h"
#include "ui/log_bridge.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QListWidgetItem>

namespace gimp {

LogPanel::LogPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

LogPanel::~LogPanel()
{
    if (logEventSub_ != 0) {
        EventBus::instance().unsubscribe(logEventSub_);
        logEventSub_ = 0;
    }
}

void LogPanel::connectToBridge(LogBridge* bridge)
{
    if (bridge) {
        connect(bridge, &LogBridge::logMessageReady, this, &LogPanel::onLogMessageReady);
        connect(bridge, &LogBridge::logMessagesReady, this, &LogPanel::onLogMessagesReady);
    }
    // TODO: handle disconnection if bridge is nullptr
}

void LogPanel::addLogMessage(const LogMessage& message)
{
    // Add to storage
    allMessages_.push_back(message);

    // Enforce maximum entries
    if (maxEntries_ > 0 && allMessages_.size() > maxEntries_) {
        allMessages_.erase(allMessages_.begin());
    }

    // Update UI if message passes current filter
    if (shouldShowMessage(message)) {
        logList_->addItem(createItemForMessage(message));
        // Scroll to bottom
        logList_->scrollToBottom();
    }
}

void LogPanel::addLogMessages(const std::vector<LogMessage>& messages)
{
    // Batch addition for efficiency
    for (const auto& msg : messages) {
        allMessages_.push_back(msg);
    }

    // Trim if needed
    if (maxEntries_ > 0 && allMessages_.size() > maxEntries_) {
        allMessages_.erase(allMessages_.begin(), allMessages_.begin() + (allMessages_.size() - maxEntries_));
    }

    // Refresh entire visible list
    refreshVisibleItems();
}

void LogPanel::clear()
{
    allMessages_.clear();
    logList_->clear();
}

void LogPanel::setMaxEntries(std::size_t max)
{
    maxEntries_ = max;
    if (maxEntries_ > 0 && allMessages_.size() > maxEntries_) {
        allMessages_.erase(allMessages_.begin(), allMessages_.begin() + (allMessages_.size() - maxEntries_));
        refreshVisibleItems();
    }
}

std::size_t LogPanel::entryCount() const
{
    return allMessages_.size();
}

void LogPanel::onFilterChanged(int index)
{
    currentFilter_ = static_cast<FilterLevel>(index);
    refreshVisibleItems();
}

void LogPanel::onClearClicked()
{
    clear();
}

void LogPanel::onCopyClicked()
{
    QString text;
    const auto selectedItems = logList_->selectedItems();
    if (selectedItems.isEmpty()) {
        // Copy all visible items
        for (int i = 0; i < logList_->count(); ++i) {
            text += logList_->item(i)->text() + '\n';
        }
    } else {
        // Copy selected items
        for (const auto* item : selectedItems) {
            text += item->text() + '\n';
        }
    }

    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        emit entriesCopied(text);
    }
}

void LogPanel::onLogMessageReady(const LogMessage& message)
{
    addLogMessage(message);
}

void LogPanel::onLogMessagesReady(const std::vector<LogMessage>& messages)
{
    addLogMessages(messages);
}

void LogPanel::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(4);

    // Button row
    buttonLayout_ = new QHBoxLayout();
    buttonLayout_->setSpacing(4);

    filterCombo_ = new QComboBox();
    filterCombo_->addItem("All");
    filterCombo_->addItem("Warnings+");
    filterCombo_->addItem("Errors only");
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogPanel::onFilterChanged);

    clearButton_ = new QPushButton("Clear");
    connect(clearButton_, &QPushButton::clicked, this, &LogPanel::onClearClicked);

    copyButton_ = new QPushButton("Copy");
    connect(copyButton_, &QPushButton::clicked, this, &LogPanel::onCopyClicked);

    buttonLayout_->addWidget(filterCombo_);
    buttonLayout_->addStretch();
    buttonLayout_->addWidget(clearButton_);
    buttonLayout_->addWidget(copyButton_);

    mainLayout_->addLayout(buttonLayout_);

    // Log list
    logList_ = new QListWidget();
    logList_->setAlternatingRowColors(true);
    logList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mainLayout_->addWidget(logList_);
}

void LogPanel::refreshVisibleItems()
{
    logList_->clear();
    for (const auto& msg : allMessages_) {
        if (shouldShowMessage(msg)) {
            logList_->addItem(createItemForMessage(msg));
        }
    }
    logList_->scrollToBottom();
}

bool LogPanel::shouldShowMessage(const LogMessage& msg) const
{
    switch (currentFilter_) {
        case FilterAll:
            return true;
        case FilterWarningsAndAbove:
            return msg.severity >= LogSeverity::Warning;
        case FilterErrorsOnly:
            return msg.severity >= LogSeverity::Error;
        default:
            return true;
    }
}

QListWidgetItem* LogPanel::createItemForMessage(const LogMessage& msg)
{
    auto* item = new QListWidgetItem(msg.formattedLine().c_str());
    // Set color based on severity
    item->setForeground(QColor(severityColor(msg.severity)));
    // TODO: set icon if we have severity icons
    return item;
}

QString LogPanel::severityIconName(LogSeverity severity) const
{
    // Return SVG icon name from resources
    switch (severity) {
        case LogSeverity::Error:
        case LogSeverity::Critical:
            return "error";
        case LogSeverity::Warning:
            return "warning";
        case LogSeverity::Info:
            return "info";
        default:
            return "debug";
    }
}

QString LogPanel::severityColor(LogSeverity severity) const
{
    switch (severity) {
        case LogSeverity::Critical:
            return "#ff0000";  // red
        case LogSeverity::Error:
            return "#ff5555";  // light red
        case LogSeverity::Warning:
            return "#ffaa00";  // orange
        case LogSeverity::Info:
            return "#0066cc";  // blue - better contrast on gray background
        case LogSeverity::Debug:
            return "#555555";  // gray
        case LogSeverity::Trace:
            return "#888888";  // light gray
        default:
            return "#000000";
    }
}

}  // namespace gimp
