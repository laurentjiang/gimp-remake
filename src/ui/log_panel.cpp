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

#include <cstddef>

namespace gimp {

LogPanel::LogPanel(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

LogPanel::~LogPanel()
{
    if (m_logEventSub != 0) {
        EventBus::instance().unsubscribe(m_logEventSub);
        m_logEventSub = 0;
    }
}

void LogPanel::connectToBridge(LogBridge* bridge)
{
    // Disconnect from previous bridge if any
    if (m_connectedBridge) {
        disconnect(m_connectedBridge, nullptr, this, nullptr);
        m_connectedBridge = nullptr;
    }

    if (bridge) {
        connect(bridge, &LogBridge::logMessageReady, this, &LogPanel::onLogMessageReady);
        connect(bridge, &LogBridge::logMessagesReady, this, &LogPanel::onLogMessagesReady);
        m_connectedBridge = bridge;
    }
}

void LogPanel::addLogMessage(const LogMessage& message)
{
    // Add to storage
    m_allMessages.push_back(message);

    // Enforce maximum entries
    if (m_maxEntries > 0 && m_allMessages.size() > m_maxEntries) {
        m_allMessages.erase(m_allMessages.begin());
    }

    // Update UI if message passes current filter
    if (shouldShowMessage(message)) {
        m_logList->addItem(createItemForMessage(message));
        // Scroll to bottom
        m_logList->scrollToBottom();
    }
}

void LogPanel::addLogMessages(const std::vector<LogMessage>& messages)
{
    // Batch addition for efficiency
    for (const auto& msg : messages) {
        m_allMessages.push_back(msg);
    }

    // Trim if needed
    if (m_maxEntries > 0 && m_allMessages.size() > m_maxEntries) {
        m_allMessages.erase(m_allMessages.begin(),
                            m_allMessages.begin() +
                                static_cast<std::ptrdiff_t>(m_allMessages.size() - m_maxEntries));
    }

    // Refresh entire visible list
    refreshVisibleItems();
}

void LogPanel::clear()
{
    m_allMessages.clear();
    m_logList->clear();
}

void LogPanel::setMaxEntries(std::size_t max)
{
    m_maxEntries = max;
    if (m_maxEntries > 0 && m_allMessages.size() > m_maxEntries) {
        m_allMessages.erase(m_allMessages.begin(),
                            m_allMessages.begin() +
                                static_cast<std::ptrdiff_t>(m_allMessages.size() - m_maxEntries));
        refreshVisibleItems();
    }
}

std::size_t LogPanel::entryCount() const
{
    return m_allMessages.size();
}

void LogPanel::onFilterChanged(int index)
{
    m_currentFilter = static_cast<FilterLevel>(index);
    refreshVisibleItems();
}

void LogPanel::onClearClicked()
{
    clear();
}

void LogPanel::onCopyClicked()
{
    QString text;
    const auto selectedItems = m_logList->selectedItems();
    if (selectedItems.isEmpty()) {
        // Copy all visible items
        for (int i = 0; i < m_logList->count(); ++i) {
            text += m_logList->item(i)->text() + '\n';
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
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    m_mainLayout->setSpacing(4);

    // Button row
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(4);

    m_filterCombo = new QComboBox();
    m_filterCombo->addItem("All");
    m_filterCombo->addItem("Warnings+");
    m_filterCombo->addItem("Errors only");
    connect(m_filterCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &LogPanel::onFilterChanged);

    m_clearButton = new QPushButton("Clear");
    connect(m_clearButton, &QPushButton::clicked, this, &LogPanel::onClearClicked);

    m_copyButton = new QPushButton("Copy");
    connect(m_copyButton, &QPushButton::clicked, this, &LogPanel::onCopyClicked);

    m_buttonLayout->addWidget(m_filterCombo);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_clearButton);
    m_buttonLayout->addWidget(m_copyButton);

    m_mainLayout->addLayout(m_buttonLayout);

    // Log list
    m_logList = new QListWidget();
    m_logList->setAlternatingRowColors(true);
    m_logList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_mainLayout->addWidget(m_logList);
}

void LogPanel::refreshVisibleItems()
{
    m_logList->clear();
    for (const auto& msg : m_allMessages) {
        if (shouldShowMessage(msg)) {
            m_logList->addItem(createItemForMessage(msg));
        }
    }
    m_logList->scrollToBottom();
}

bool LogPanel::shouldShowMessage(const LogMessage& msg) const
{
    switch (m_currentFilter) {
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
