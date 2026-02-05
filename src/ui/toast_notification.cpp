/**
 * @file toast_notification.cpp
 * @brief Implementation of ToastNotification widget
 * @author Laurent Jiang
 * @date 2026-02-05
 */

#include "ui/toast_notification.h"

#include "ui/toast_constants.h"

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

#include <utility>

namespace gimp {

ToastNotification::ToastNotification(LogMessage message, QWidget* parent)
    : QWidget(parent),
      m_message(std::move(message))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Fixed width, height determined by content
    setFixedWidth(toast::kToastWidth);
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
    m_autoDismissTimer = new QTimer(this);
    m_autoDismissTimer->setSingleShot(true);
    connect(m_autoDismissTimer, &QTimer::timeout, this, &ToastNotification::onTimeout);

    // Fade animation
    m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity");
    m_fadeAnimation->setDuration(300);  // 300 ms fade
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);
    connect(
        m_fadeAnimation, &QPropertyAnimation::finished, this, &ToastNotification::onFadeFinished);
}

ToastNotification::~ToastNotification() = default;

void ToastNotification::setupUi()
{
    // Main horizontal layout with proper margins
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // Icon label - use QIcon for proper SVG rendering
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(24, 24);
    QIcon icon(severityIcon());
    m_iconLabel->setPixmap(icon.pixmap(24, 24));
    layout->addWidget(m_iconLabel);

    // Text label with word wrap
    m_textLabel = new QLabel(QString::fromStdString(m_message.message), this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setStyleSheet("color: white; font-size: 12px;");
    m_textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(m_textLabel);

    // Set minimum height to ensure icon is never clipped
    setMinimumHeight(48);
}

void ToastNotification::showToast()
{
    show();
    raise();

    // Start auto-dismiss timer based on severity
    int timeoutMs = 0;
    switch (m_message.severity) {
        case LogSeverity::Info:
            timeoutMs = toast::kInfoTimeoutMs;  // 3 seconds
            break;
        case LogSeverity::Warning:
            timeoutMs = toast::kWarningTimeoutMs;  // 5 seconds
            break;
        case LogSeverity::Error:
        case LogSeverity::Critical:
            // Persistent - no auto-dismiss
            return;
        default:
            timeoutMs = toast::kInfoTimeoutMs;
    }

    if (timeoutMs > 0) {
        m_autoDismissTimer->start(timeoutMs);
    }
}

int ToastNotification::totalHeight() const
{
    return height();
}

void ToastNotification::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    if (m_autoDismissTimer->isActive()) {
        m_autoDismissTimer->stop();
    }
    QWidget::enterEvent(event);
}

void ToastNotification::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    // Restart timer only if severity is not persistent
    if (m_message.severity != LogSeverity::Error && m_message.severity != LogSeverity::Critical) {
        int remaining = 0;
        switch (m_message.severity) {
            case LogSeverity::Info:
                remaining = toast::kInfoTimeoutMs;
                break;
            case LogSeverity::Warning:
                remaining = toast::kWarningTimeoutMs;
                break;
            default:
                remaining = toast::kInfoTimeoutMs;
        }
        if (remaining > 0) {
            m_autoDismissTimer->start(remaining);
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
    path.addRoundedRect(rect(), 6, 6);  // slightly smaller radius

    // Solid background with alpha
    QColor baseColor(severityColor());
    baseColor.setAlpha(230);  // 90% opacity
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
    close();  // Will delete because of WA_DeleteOnClose
}

void ToastNotification::startFadeOut()
{
    if (m_fadeAnimation->state() == QPropertyAnimation::Running) {
        return;
    }
    m_fadeAnimation->start();
}

QString ToastNotification::severityColor() const
{
    // Muted colors aligned with dark theme
    switch (m_message.severity) {
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
    switch (m_message.severity) {
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
