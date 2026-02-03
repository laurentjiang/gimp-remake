/**
 * @file spin_slider.cpp
 * @brief Implementation of SpinSlider widget.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#include "ui/spin_slider.h"

#include "ui/theme.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOption>
#include <QWheelEvent>

namespace gimp {

namespace {
// Height of the spin slider widget
constexpr int kSliderHeight = 20;
// Border radius for rounded corners
constexpr int kBorderRadius = 3;
// Horizontal padding for text
constexpr int kTextPadding = 6;
}  // namespace

SpinSlider::SpinSlider(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(kSliderHeight);
    setMaximumHeight(kSliderHeight);
    setMinimumWidth(120);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    editor_ = new QLineEdit(this);
    editor_->hide();
    editor_->setAlignment(Qt::AlignCenter);
    editor_->setFrame(false);
    connect(editor_, &QLineEdit::editingFinished, this, &SpinSlider::onEditFinished);
}

void SpinSlider::setMinimum(double min)
{
    min_ = min;
    if (value_ < min_) {
        setValue(min_);
    }
    update();
}

void SpinSlider::setMaximum(double max)
{
    max_ = max;
    if (value_ > max_) {
        setValue(max_);
    }
    update();
}

void SpinSlider::setRange(double min, double max)
{
    min_ = min;
    max_ = max;
    value_ = std::clamp(value_, min_, max_);
    update();
}

void SpinSlider::setValue(double value)
{
    double newValue = std::clamp(value, min_, max_);
    if (qFuzzyCompare(newValue, value_)) {
        return;
    }
    value_ = newValue;
    update();
    emit valueChanged(value_);
}

void SpinSlider::setSuffix(const QString& suffix)
{
    suffix_ = suffix;
    update();
}

void SpinSlider::setDecimals(int decimals)
{
    decimals_ = decimals;
    update();
}

void SpinSlider::setSingleStep(double step)
{
    step_ = step;
}

void SpinSlider::setLabel(const QString& label)
{
    label_ = label;
    update();
}

QString SpinSlider::formatValue() const
{
    return QString::number(value_, 'f', decimals_) + suffix_;
}

void SpinSlider::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect rect = this->rect().adjusted(1, 1, -1, -1);

    // Background with rounded corners
    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.setBrush(Theme::toQColor(Theme::kSliderBackground));
    painter.drawRoundedRect(rect, kBorderRadius, kBorderRadius);

    // Fill bar based on value
    if (max_ > min_) {
        double ratio = (value_ - min_) / (max_ - min_);
        int fillWidth = static_cast<int>(static_cast<double>(rect.width() - 2) * ratio);
        if (fillWidth > 0) {
            QRect fillRect(rect.x() + 1, rect.y() + 1, fillWidth, rect.height() - 2);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::toQColor(Theme::kSliderFill));
            painter.drawRoundedRect(fillRect, kBorderRadius - 1, kBorderRadius - 1);
        }
    }

    // Text
    painter.setPen(Theme::toQColor(Theme::kTextPrimary));
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    QRect textRect = rect.adjusted(kTextPadding, 0, -kTextPadding, 0);

    // Label on left, value on right
    if (!label_.isEmpty()) {
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label_);
    }
    painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, formatValue());
}

void SpinSlider::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        updateValueFromPosition(event->pos().x());
    }
}

void SpinSlider::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_) {
        updateValueFromPosition(event->pos().x());
    }
}

void SpinSlider::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
    }
}

void SpinSlider::wheelEvent(QWheelEvent* event)
{
    int delta = event->angleDelta().y();
    if (delta > 0) {
        setValue(value_ + step_);
    } else if (delta < 0) {
        setValue(value_ - step_);
    }
    event->accept();
}

void SpinSlider::mouseDoubleClickEvent(QMouseEvent* /*event*/)
{
    showEditor();
}

void SpinSlider::updateValueFromPosition(int x)
{
    int margin = 2;
    int usableWidth = width() - 2 * margin;
    if (usableWidth <= 0) {
        return;
    }

    double ratio = static_cast<double>(x - margin) / static_cast<double>(usableWidth);
    ratio = std::clamp(ratio, 0.0, 1.0);
    double newValue = min_ + ratio * (max_ - min_);

    // Snap to step increments
    if (step_ > 0) {
        newValue = std::round(newValue / step_) * step_;
    }

    setValue(newValue);
}

void SpinSlider::showEditor()
{
    editor_->setGeometry(rect().adjusted(2, 2, -2, -2));
    editor_->setText(QString::number(value_, 'f', decimals_));
    editor_->selectAll();
    editor_->show();
    editor_->setFocus();
}

void SpinSlider::hideEditor()
{
    editor_->hide();
    setFocus();
}

void SpinSlider::onEditFinished()
{
    bool ok = false;
    double newValue = editor_->text().toDouble(&ok);
    if (ok) {
        setValue(newValue);
    }
    hideEditor();
}

}  // namespace gimp
