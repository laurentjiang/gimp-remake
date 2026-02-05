/**
 * @file toast_notification.cpp
 * @brief Implementation of ToastNotification widget
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/toast_notification.h"

#include <QApplication>
#include <QEnterEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

namespace gimp {

ToastNotification::ToastNotification(const LogMessage& message, QWidget* parent)
    : QWidget(parent)
    , message_(message)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Fixed width, height determined by content
    setFixedWidth(280);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    // Drop shadow - more subtle
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(6);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 1);
    setGraphicsEffect(shadow);

    setupUi();

    // Force layout calculation
    adjustSize();

    // Timer for auto-dismiss
    autoDismissTimer_ = new QTimer(this);
    autoDismissTimer_->setSingleShot(true);
    connect(autoDismissTimer_, &QTimer::timeout, this, &ToastNotification::onTimeout);

    // Fade animation
    fadeAnimation_ = new QPropertyAnimation(this, "windowOpacity");
    fadeAnimation_->setDuration(300); // 300 ms fade
    fadeAnimation_->setStartValue(1.0);
    fadeAnimation_->setEndValue(0.0);
    connect(fadeAnimation_, &QPropertyAnimation::finished, this, &ToastNotification::onFadeFinished);
}

ToastNotification::~ToastNotification() = default;

void ToastNotification::setupUi()
{
    // Main horizontal layout with proper margins
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // Icon label - use QIcon for proper SVG rendering
    iconLabel_ = new QLabel(this);
    iconLabel_->setFixedSize(24, 24);
    QIcon icon(severityIcon());
    iconLabel_->setPixmap(icon.pixmap(24, 24));
    layout->addWidget(iconLabel_);

    // Text label with word wrap
    textLabel_ = new QLabel(QString::fromStdString(message_.message), this);
    textLabel_->setWordWrap(true);
    textLabel_->setStyleSheet("color: white; font-size: 12px;");
    textLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(textLabel_);

    // Set minimum height to ensure icon is never clipped
    setMinimumHeight(48);
}

void ToastNotification::showToast()
{
    show();
    raise();

    // Start auto-dismiss timer based on severity
    int timeoutMs = 0;
    switch (message_.severity) {
        case LogSeverity::Info:
            timeoutMs = 3000; // 3 seconds
            break;
        case LogSeverity::Warning:
            timeoutMs = 5000; // 5 seconds
            break;
        case LogSeverity::Error:
        case LogSeverity::Critical:
            // Persistent - no auto-dismiss
            return;
        default:
            timeoutMs = 3000;
    }

    if (timeoutMs > 0) {
        autoDismissTimer_->start(timeoutMs);
    }
}

int ToastNotification::totalHeight() const
{
    return height();
}

void ToastNotification::enterEvent(QEnterEvent* event)
{
    isHovered_ = true;
    if (autoDismissTimer_->isActive()) {
        autoDismissTimer_->stop();
    }
    QWidget::enterEvent(event);
}

void ToastNotification::leaveEvent(QEvent* event)
{
    isHovered_ = false;
    // Restart timer only if severity is not persistent
    if (message_.severity != LogSeverity::Error && message_.severity != LogSeverity::Critical) {
        int remaining = 0;
        switch (message_.severity) {
            case LogSeverity::Info:
                remaining = 3000;
                break;
            case LogSeverity::Warning:
                remaining = 5000;
                break;
            default:
                remaining = 3000;
        }
        if (remaining > 0) {
            autoDismissTimer_->start(remaining);
        }
    }
    QWidget::leaveEvent(event);
}

void ToastNotification::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        startFadeOut();
    }
    QWidget::mousePressEvent(event);
}

void ToastNotification::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rounded rectangle
    QPainterPath path;
    path.addRoundedRect(rect(), 6, 6); // slightly smaller radius

    // Solid background with alpha
    QColor baseColor(severityColor());
    baseColor.setAlpha(230); // 90% opacity
    painter.fillPath(path, baseColor);

    // Border - subtle
    painter.setPen(QPen(baseColor.darker(130), 1));
    painter.drawPath(path);

    QWidget::paintEvent(event);
}

void ToastNotification::onTimeout()
{
    startFadeOut();
}

void ToastNotification::onFadeFinished()
{
    emit dismissed(this);
    close(); // Will delete because of WA_DeleteOnClose
}

void ToastNotification::startFadeOut()
{
    if (fadeAnimation_->state() == QPropertyAnimation::Running) {
        return;
    }
    fadeAnimation_->start();
}

QString ToastNotification::severityColor() const
{
    // Muted colors aligned with dark theme
    switch (message_.severity) {
        case LogSeverity::Critical:
            return "#8B0000";  // dark red
        case LogSeverity::Error:
            return "#A52A2A";  // brownish red
        case LogSeverity::Warning:
            return "#CC8400";  // muted orange
        case LogSeverity::Info:
            return "#2E5EAA";  // muted blue
        case LogSeverity::Debug:
            return "#555555";  // gray
        case LogSeverity::Trace:
            return "#666666";  // lighter gray
        default:
            return "#444444";
    }
}

QString ToastNotification::severityIcon() const
{
    // Return SVG icon path from resources
    switch (message_.severity) {
        case LogSeverity::Error:
        case LogSeverity::Critical:
            return ":/icons/error.svg";
        case LogSeverity::Warning:
            return ":/icons/warning.svg";
        case LogSeverity::Info:
            return ":/icons/info.svg";
        default:
            return ":/icons/debug.svg";
    }
}

void ToastNotification::dismiss()
{
    startFadeOut();
}

}  // namespace gimp
