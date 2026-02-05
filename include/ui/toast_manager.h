/**
 * @file toast_manager.h
 * @brief Manages a stack of toast notifications in the UI
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

#include "ui/log_message.h"
#include "ui/toast_constants.h"
#include "ui/toast_notification.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <chrono>
#include <vector>

namespace gimp {

class LogBridge;

/**
 * @brief Manages a vertical stack of toast notifications
 *
 * ToastManager positions toast widgets in a corner of a parent widget
 * (default top‑right) and ensures they don't overflow the screen.
 * It connects to LogBridge signals to automatically show toasts for
 * user‑facing log messages.
 *
 * Features:
 * - Limits maximum visible toasts (default 5)
 * - Automatically removes dismissed toasts
 * - Can be enabled/disabled globally
 * - Supports configurable timeouts per severity
 */
class ToastManager : public QObject {
    Q_OBJECT

  public:
    /**
     * @brief Construct a ToastManager
     * @param parentWidget The widget that will host the toasts (e.g., MainWindow)
     * @param parent Optional parent QObject
     */
    explicit ToastManager(QWidget* parentWidget, QObject* parent = nullptr);
    ~ToastManager() override;

    /**
     * @brief Connect to a LogBridge for automatic toast display
     * @param bridge The LogBridge instance (can be nullptr to disconnect)
     */
    void connectToBridge(LogBridge* bridge);

    /**
     * @brief Show a toast for the given log message
     * @param message The log message to display
     *
     * If the maximum toast limit is reached, the oldest toast is dismissed.
     */
    void showToast(const LogMessage& message);

    /**
     * @brief Clear all visible toasts immediately
     */
    void clearAll();

    /**
     * @brief Set the maximum number of toasts visible at once
     * @param max Maximum toasts (0 = unlimited)
     */
    void setMaxToasts(int max) { maxToasts_ = max; }

    /**
     * @brief Get the current maximum toast limit
     */
    int maxToasts() const { return maxToasts_; }

    /**
     * @brief Enable or disable toast display globally
     * @param enabled True to enable, false to suppress all toasts
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if toast display is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Set the corner where toasts will appear
     * @param corner Qt::Corner (TopLeft, TopRight, BottomLeft, BottomRight)
     */
    void setCorner(Qt::Corner corner);

    /**
     * @brief Get the current corner
     */
    Qt::Corner corner() const { return corner_; }

  signals:
    /**
     * @brief Emitted when a toast is shown
     * @param message The message that was displayed
     */
    void toastShown(const LogMessage& message);

    /**
     * @brief Emitted when a toast is dismissed (either by user or timeout)
     * @param message The message that was dismissed
     */
    void toastDismissed(const LogMessage& message);

  public slots:
    /**
     * @brief Update positions of all toasts (e.g., after window resize)
     */
    void repositionToasts();

  private slots:
    void onLogMessageReady(const LogMessage& message);
    void onToastDismissed(ToastNotification* toast);

  private:
    void addToast(ToastNotification* toast);
    void removeToast(ToastNotification* toast);
    QPoint calculateToastPosition(int index) const;
    void enforceLimit();

    QPointer<QWidget> parentWidget_;
    LogBridge* m_connectedBridge = nullptr;  ///< Currently connected bridge (if any)
    std::vector<ToastNotification*> toasts_;
    int maxToasts_ = toast::kMaxToasts;
    bool enabled_ = true;
    Qt::Corner corner_ = Qt::BottomRightCorner;
    int margin_ = toast::kToastMargin;
    int spacing_ = toast::kToastSpacing;

    // Rate‑limiting state
    std::chrono::steady_clock::time_point lastToastTime_;
    int toastsThisSecond_ = 0;
};

}  // namespace gimp
