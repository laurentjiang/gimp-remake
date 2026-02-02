/**
 * @file spin_slider.cpp
 * @brief Implementation of SpinSlider widget.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#include "ui/spin_slider.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOption>
#include <QWheelEvent>

namespace gimp {

SpinSlider::SpinSlider(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(24);
    setMaximumHeight(24);
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

    QRect rect = this->rect().adjusted(1, 1, -1, -1);

    // Background
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    painter.setBrush(QColor(60, 60, 60));
    painter.drawRect(rect);

    // Fill bar based on value
    if (max_ > min_) {
        double ratio = (value_ - min_) / (max_ - min_);
        int fillWidth = static_cast<int>(static_cast<double>(rect.width() - 2) * ratio);
        if (fillWidth > 0) {
            QRect fillRect(rect.x() + 1, rect.y() + 1, fillWidth, rect.height() - 2);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(85, 85, 85));
            painter.drawRect(fillRect);
        }
    }

    // Text
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    QString text;
    if (!label_.isEmpty()) {
        text = label_ + ": " + formatValue();
    } else {
        text = formatValue();
    }

    painter.drawText(rect, Qt::AlignCenter, text);
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
