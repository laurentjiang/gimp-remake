/**
 * @file toast_manager.cpp
 * @brief Implementation of ToastManager
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/toast_manager.h"
#include "ui/log_bridge.h"

#include <QApplication>
#include <QScreen>
#include <QTimer>

namespace gimp {

ToastManager::ToastManager(QWidget* parentWidget, QObject* parent)
    : QObject(parent)
    , parentWidget_(parentWidget)
{
    // Ensure we have a valid parent widget
    if (!parentWidget_) {
        parentWidget_ = QApplication::activeWindow();
    }
}

ToastManager::~ToastManager()
{
    clearAll();
}

void ToastManager::connectToBridge(LogBridge* bridge)
{
    if (bridge) {
        connect(bridge, &LogBridge::logMessageReady,
                this, &ToastManager::onLogMessageReady,
                Qt::QueuedConnection);
    }
    // TODO: handle disconnection if bridge is nullptr
}

void ToastManager::showToast(const LogMessage& message)
{
    if (!enabled_ || !parentWidget_) {
        return;
    }

    // Create toast widget
    auto* toast = new ToastNotification(message, parentWidget_);
    connect(toast, &ToastNotification::dismissed,
            this, &ToastManager::onToastDismissed);
    connect(toast, &ToastNotification::clicked,
            [toast]() { toast->dismiss(); });

    addToast(toast);
    toast->showToast();

    emit toastShown(message);
}

void ToastManager::clearAll()
{
    for (auto* toast : toasts_) {
        toast->disconnect();
        toast->close(); // will delete because of WA_DeleteOnClose
    }
    toasts_.clear();
}

void ToastManager::setCorner(Qt::Corner corner)
{
    if (corner_ != corner) {
        corner_ = corner;
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
            showToast(message);
            break;
        default:
            // Trace, Debug, Off are not shown as toasts
            break;
    }
}

void ToastManager::onToastDismissed(ToastNotification* toast)
{
    removeToast(toast);
    emit toastDismissed(toast->message());
}

void ToastManager::repositionToasts()
{
    if (toasts_.empty()) {
        return;
    }

    // Update positions of all toasts
    for (std::size_t i = 0; i < toasts_.size(); ++i) {
        toasts_[i]->move(calculateToastPosition(static_cast<int>(i)));
    }
}

void ToastManager::addToast(ToastNotification* toast)
{
    toasts_.push_back(toast);
    enforceLimit();
    repositionToasts();
}

void ToastManager::removeToast(ToastNotification* toast)
{
    auto it = std::find(toasts_.begin(), toasts_.end(), toast);
    if (it != toasts_.end()) {
        toasts_.erase(it);
        repositionToasts();
    }
}

QPoint ToastManager::calculateToastPosition(int index) const
{
    if (!parentWidget_) {
        return QPoint(0, 0);
    }

    const QRect parentRect = parentWidget_->rect();
    const int toastWidth = 280; // matches ToastNotification::fixedWidth

    // Calculate cumulative height of all toasts before this one
    int cumulativeHeight = 0;
    for (int i = 0; i < index; ++i) {
        cumulativeHeight += toasts_[i]->totalHeight() + spacing_;
    }

    const int toastHeight = toasts_[index]->totalHeight();

    int x = 0;
    int y = 0;

    switch (corner_) {
        case Qt::TopLeftCorner:
            x = margin_;
            y = margin_ + cumulativeHeight;
            break;
        case Qt::TopRightCorner:
            x = parentRect.width() - toastWidth - margin_;
            y = margin_ + cumulativeHeight;
            break;
        case Qt::BottomLeftCorner:
            x = margin_;
            y = parentRect.height() - margin_ - cumulativeHeight - toastHeight;
            break;
        case Qt::BottomRightCorner:
            x = parentRect.width() - toastWidth - margin_;
            y = parentRect.height() - margin_ - cumulativeHeight - toastHeight;
            break;
    }

    // Toasts are now child widgets (no Qt::ToolTip), so coordinates are parent-relative.
    // Ensure position is within parent's visible area.
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + toastWidth > parentRect.width()) x = parentRect.width() - toastWidth;
    if (y + toastHeight > parentRect.height()) y = parentRect.height() - toastHeight;

    return QPoint(x, y);
}

void ToastManager::enforceLimit()
{
    if (maxToasts_ <= 0) {
        return;
    }

    while (static_cast<int>(toasts_.size()) > maxToasts_) {
        // Remove the oldest toast (first in vector)
        auto* oldest = toasts_.front();
        removeToast(oldest);
        oldest->dismiss();
    }
}

}  // namespace gimp
