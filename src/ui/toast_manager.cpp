/**
 * @file toast_manager.cpp
 * @brief Implementation of ToastManager
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/toast_manager.h"

#include "ui/log_bridge.h"
#include "ui/toast_constants.h"

#include <QApplication>
#include <QScreen>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace gimp {

ToastManager::ToastManager(QWidget* parentWidget, QObject* parent)
    : QObject(parent),
      m_parentWidget(parentWidget)
{
    // Ensure we have a valid parent widget
    if (!m_parentWidget) {
        m_parentWidget = QApplication::activeWindow();
    }
}

ToastManager::~ToastManager()
{
    clearAll();
}

void ToastManager::connectToBridge(LogBridge* bridge)
{
    // Disconnect from previous bridge if any
    if (m_connectedBridge) {
        disconnect(m_connectedBridge, nullptr, this, nullptr);
        m_connectedBridge = nullptr;
    }

    if (bridge) {
        connect(bridge,
                &LogBridge::logMessageReady,
                this,
                &ToastManager::onLogMessageReady,
                Qt::QueuedConnection);
        m_connectedBridge = bridge;
    }
}

void ToastManager::showToast(const LogMessage& message)
{
    if (!m_enabled || !m_parentWidget) {
        return;
    }

    // Create toast widget
    auto* toast = new ToastNotification(message, m_parentWidget);
    connect(toast, &ToastNotification::dismissed, this, &ToastManager::onToastDismissed);
    connect(toast, &ToastNotification::clicked, [toast]() { toast->dismiss(); });

    addToast(toast);
    toast->showToast();

    emit toastShown(message);
}

void ToastManager::clearAll()
{
    for (auto* toast : m_toasts) {
        toast->disconnect();
        toast->close();  // will delete because of WA_DeleteOnClose
    }
    m_toasts.clear();
}

void ToastManager::setCorner(Qt::Corner corner)
{
    if (m_corner != corner) {
        m_corner = corner;
        repositionToasts();
    }
}

void ToastManager::onLogMessageReady(const LogMessage& message)
{
    // Only show toasts for user-facing severities
    switch (message.severity) {
        case LogSeverity::Info:
        case LogSeverity::Warning:
        case LogSeverity::Error:
        case LogSeverity::Critical:
            // These severities are shown as toasts
            break;
        default:
            // Trace, Debug, Off are not shown as toasts
            return;
    }

    // Apply rate limiting (max 3 toasts per second)
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastToastTime).count();

    if (elapsed >= 1000) {
        // New second, reset counter
        m_toastsThisSecond = 0;
        m_lastToastTime = now;
    }

    if (m_toastsThisSecond >= toast::kMaxToastsPerSecond) {
        // Limit reached, skip this toast
        return;
    }

    ++m_toastsThisSecond;
    showToast(message);
}

void ToastManager::onToastDismissed(ToastNotification* toast)
{
    // Disconnect all signals from this toast to avoid QObject warnings
    toast->disconnect();
    removeToast(toast);
    emit toastDismissed(toast->message());
}

void ToastManager::repositionToasts()
{
    if (m_toasts.empty()) {
        return;
    }

    // Update positions of all toasts
    for (std::size_t i = 0; i < m_toasts.size(); ++i) {
        m_toasts[i]->move(calculateToastPosition(static_cast<int>(i)));
    }
}

void ToastManager::addToast(ToastNotification* toast)
{
    m_toasts.push_back(toast);
    enforceLimit();
    repositionToasts();
}

void ToastManager::removeToast(ToastNotification* toast)
{
    auto it = std::find(m_toasts.begin(), m_toasts.end(), toast);
    if (it != m_toasts.end()) {
        m_toasts.erase(it);
        repositionToasts();
    }
}

QPoint ToastManager::calculateToastPosition(int index) const
{
    if (!m_parentWidget) {
        return {0, 0};
    }

    const QRect parentRect = m_parentWidget->rect();
    const int toastWidth = toast::kToastWidth;  // matches ToastNotification::fixedWidth

    // Calculate cumulative height of all toasts before this one
    int cumulativeHeight = 0;
    for (int i = 0; i < index; ++i) {
        cumulativeHeight += m_toasts[i]->totalHeight() + m_spacing;
    }

    const int toastHeight = m_toasts[index]->totalHeight();

    int x = 0;
    int y = 0;

    switch (m_corner) {
        case Qt::TopLeftCorner:
            x = m_margin;
            y = m_margin + cumulativeHeight;
            break;
        case Qt::TopRightCorner:
            x = parentRect.width() - toastWidth - m_margin;
            y = m_margin + cumulativeHeight;
            break;
        case Qt::BottomLeftCorner:
            x = m_margin;
            y = parentRect.height() - m_margin - cumulativeHeight - toastHeight;
            break;
        case Qt::BottomRightCorner:
            x = parentRect.width() - toastWidth - m_margin;
            y = parentRect.height() - m_margin - cumulativeHeight - toastHeight;
            break;
    }

    // Toasts are now child widgets (no Qt::ToolTip), so coordinates are parent-relative.
    // Ensure position is within parent's visible area.
    x = std::max(x, 0);
    y = std::max(y, 0);
    if (x + toastWidth > parentRect.width())
        x = parentRect.width() - toastWidth;
    if (y + toastHeight > parentRect.height())
        y = parentRect.height() - toastHeight;

    return {x, y};
}

void ToastManager::enforceLimit()
{
    if (m_maxToasts <= 0) {
        return;
    }

    while (m_toasts.size() > static_cast<std::size_t>(m_maxToasts)) {
        // Remove the oldest toast (first in vector)
        auto* oldest = m_toasts.front();
        removeToast(oldest);
        oldest->dismiss();
    }
}

}  // namespace gimp
