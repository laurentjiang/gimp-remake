/**
 * @file toast_notification.h
 * @brief Transient pop-up notification widget
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#pragma once

#include "ui/log_message.h"

#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

namespace gimp {

/**
 * @brief Single toast notification widget
 *
 * Displays a message with a severity icon and optional close button.
 * Automatically fades out after a configurable timeout (depending on severity).
 * Can be dismissed by clicking or hovering (pauses timer).
 */
class ToastNotification : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Construct a toast notification widget
     * @param message The log message to display
     * @param parent Optional parent widget
     */
    explicit ToastNotification(const LogMessage& message, QWidget* parent = nullptr);
    ~ToastNotification() override;

    /**
     * @brief Start the toast lifecycle (show, start timer, auto‑dismiss)
     */
    void showToast();

    /**
     * @brief Get the toast's height including margins
     */
    int totalHeight() const;

    /**
     * @brief Get the toast's message
     */
    const LogMessage& message() const { return message_; }

    /**
     * @brief Programmatically dismiss the toast (start fade-out)
     */
    void dismiss();

  signals:
    /**
     * @brief Emitted when the toast is about to be destroyed (after fade‑out)
     * @param toast Pointer to this toast (for container cleanup)
     */
    void dismissed(ToastNotification* toast);

    /**
     * @brief Emitted when the toast is clicked
     */
    void clicked();

  protected:
    /** @brief Handle mouse enter events to pause auto-dismiss timer. */
    void enterEvent(QEnterEvent* event) override;
    /** @brief Handle mouse leave events to resume auto-dismiss timer. */
    void leaveEvent(QEvent* event) override;
    /** @brief Handle mouse clicks to emit clicked() signal. */
    void mousePressEvent(QMouseEvent* event) override;
    /** @brief Paint the toast with rounded corners and gradient background. */
    void paintEvent(QPaintEvent* event) override;

  private slots:
    void onTimeout();
    void onFadeFinished();

  private:
    void setupUi();
    void startFadeOut();
    QString severityColor() const;
    QString severityIcon() const;

    LogMessage message_;
    QTimer* autoDismissTimer_ = nullptr;
    QPropertyAnimation* fadeAnimation_ = nullptr;
    QLabel* iconLabel_ = nullptr;
    QLabel* textLabel_ = nullptr;
    bool isHovered_ = false;
};

}  // namespace gimp
