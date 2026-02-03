/**
 * @file tool_button.cpp
 * @brief Implementation of ToolButton widget.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#include "ui/tool_button.h"

#include "ui/theme.h"

#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>

namespace gimp {

namespace {
constexpr int kButtonSize = 24;
constexpr int kIconSize = 18;
constexpr int kTriangleSize = 5;
}  // namespace

ToolButton::ToolButton(const ToolDescriptor& descriptor, QWidget* parent)
    : QToolButton(parent),
      primaryTool_(descriptor),
      currentToolId_(descriptor.id)
{
    setFixedSize(kButtonSize, kButtonSize);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setToolTip(QString::fromStdString(
        descriptor.name + (descriptor.shortcut.empty() ? "" : " (" + descriptor.shortcut + ")")));

    const QIcon icon(QString::fromStdString(descriptor.iconName));
    setIcon(icon);
    setIconSize(QSize(kIconSize, kIconSize));

    updateAppearance();
}

void ToolButton::setSubTools(const std::vector<ToolDescriptor>& subTools)
{
    subTools_ = subTools;

    if (!subTools_.empty()) {
        subToolMenu_ = new QMenu(this);
        subToolMenu_->setStyleSheet(
            QString("QMenu { background-color: %1; color: %2; border: 1px solid %3; }"
                    "QMenu::item { padding: 4px 20px; }"
                    "QMenu::item:selected { background-color: %4; }")
                .arg(Theme::toHex(Theme::kPanelBackground),
                     Theme::toHex(Theme::kTextPrimary),
                     Theme::toHex(Theme::kBorderLight),
                     Theme::toHex(Theme::kSliderBackground)));

        for (const auto& tool : subTools_) {
            QAction* action = subToolMenu_->addAction(QIcon(QString::fromStdString(tool.iconName)),
                                                      QString::fromStdString(tool.name));
            action->setData(QString::fromStdString(tool.id));
            connect(action, &QAction::triggered, this, [this, toolId = tool.id]() {
                setCurrentTool(toolId);
                emit toolActivated(toolId);
            });
        }
    }

    update();
}

void ToolButton::setCurrentTool(const std::string& toolId)
{
    if (currentToolId_ == toolId) {
        return;
    }

    currentToolId_ = toolId;

    // Find the tool descriptor and update icon
    if (toolId == primaryTool_.id) {
        setIcon(QIcon(QString::fromStdString(primaryTool_.iconName)));
        setToolTip(QString::fromStdString(
            primaryTool_.name +
            (primaryTool_.shortcut.empty() ? "" : " (" + primaryTool_.shortcut + ")")));
    } else {
        for (const auto& tool : subTools_) {
            if (tool.id == toolId) {
                setIcon(QIcon(QString::fromStdString(tool.iconName)));
                setToolTip(QString::fromStdString(
                    tool.name + (tool.shortcut.empty() ? "" : " (" + tool.shortcut + ")")));
                break;
            }
        }
    }

    update();
}

void ToolButton::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect rect = this->rect();

    // Background only when hovered or checked
    if (isChecked() || hovered_) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::toQColor(Theme::kSliderBackground));
        painter.drawRect(rect);
    }

    // Draw icon centered using iconSize for correct positioning
    QIcon currentIcon = icon();
    if (!currentIcon.isNull()) {
        QSize icoSize = iconSize();
        QPixmap pixmap = currentIcon.pixmap(icoSize);
        int x = (rect.width() - icoSize.width()) / 2;
        int y = (rect.height() - icoSize.height()) / 2;
        painter.drawPixmap(x, y, pixmap);
    }

    // Draw triangle indicator for sub-tools
    if (!subTools_.empty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::toQColor(Theme::kTextPrimary));

        QPolygon triangle;
        int x = rect.right() - kTriangleSize - 1;
        int y = rect.bottom() - kTriangleSize - 1;
        triangle << QPoint(x + kTriangleSize, y) << QPoint(x + kTriangleSize, y + kTriangleSize)
                 << QPoint(x, y + kTriangleSize);
        painter.drawPolygon(triangle);
    }
}

void ToolButton::enterEvent(QEnterEvent* event)
{
    hovered_ = true;
    update();
    QToolButton::enterEvent(event);
}

void ToolButton::leaveEvent(QEvent* event)
{
    hovered_ = false;
    update();
    QToolButton::leaveEvent(event);
}

void ToolButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit toolActivated(currentToolId_);
    }
    QToolButton::mousePressEvent(event);
}

void ToolButton::contextMenuEvent(QContextMenuEvent* event)
{
    if (subToolMenu_ != nullptr && !subTools_.empty()) {
        subToolMenu_->exec(event->globalPos());
    }
}

void ToolButton::updateAppearance()
{
    // Flat style - no border
    setStyleSheet("QToolButton { border: none; background: transparent; }");
}

}  // namespace gimp
