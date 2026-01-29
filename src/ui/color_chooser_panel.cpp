/**
 * @file color_chooser_panel.cpp
 * @brief Implementation of ColorChooserPanel widget.
 * @author Laurent Jiang
 * @date 2026-01-29
 */

#include "ui/color_chooser_panel.h"

#include "core/events.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include <algorithm>
#include <cmath>

namespace gimp {

// ============================================================================
// ColorSquare Implementation
// ============================================================================

ColorSquare::ColorSquare(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(150, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(false);
}

void ColorSquare::setHue(int hue)
{
    hue_ = std::clamp(hue, 0, 359);
    update();
}

void ColorSquare::setSaturation(int saturation)
{
    saturation_ = std::clamp(saturation, 0, 255);
    update();
}

void ColorSquare::setValue(int value)
{
    value_ = std::clamp(value, 0, 255);
    update();
}

void ColorSquare::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Draw the color gradient
    QImage image(w, h, QImage::Format_RGB32);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Saturation increases left to right (0-255)
            const int s = (x * 255) / std::max(1, w - 1);
            // Value decreases top to bottom (255-0)
            const int v = 255 - (y * 255) / std::max(1, h - 1);

            int r = 0;
            int g = 0;
            int b = 0;
            ColorChooserPanel::hsvToRgb(hue_, s, v, r, g, b);

            image.setPixel(x, y, qRgb(r, g, b));
        }
    }

    painter.drawImage(0, 0, image);

    // Draw selection circle
    const int selX = (saturation_ * (w - 1)) / 255;
    const int selY = ((255 - value_) * (h - 1)) / 255;

    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPoint(selX, selY), 6, 6);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(QPoint(selX, selY), 7, 7);
}

void ColorSquare::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        updateFromPosition(event->pos());
    }
}

void ColorSquare::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        updateFromPosition(event->pos());
    }
}

void ColorSquare::updateFromPosition(const QPoint& pos)
{
    const int w = width();
    const int h = height();

    saturation_ = std::clamp((pos.x() * 255) / std::max(1, w - 1), 0, 255);
    value_ = std::clamp(255 - (pos.y() * 255) / std::max(1, h - 1), 0, 255);

    update();
    emit colorChanged(saturation_, value_);
}

// ============================================================================
// HueSlider Implementation
// ============================================================================

HueSlider::HueSlider(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(20, 150);
    setMaximumWidth(30);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void HueSlider::setHue(int hue)
{
    hue_ = std::clamp(hue, 0, 359);
    update();
}

void HueSlider::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    const int w = width();
    const int h = height();

    // Draw hue gradient
    for (int y = 0; y < h; ++y) {
        const int hue = (y * 359) / std::max(1, h - 1);
        int r = 0;
        int g = 0;
        int b = 0;
        ColorChooserPanel::hsvToRgb(hue, 255, 255, r, g, b);
        painter.setPen(QColor(r, g, b));
        painter.drawLine(0, y, w, y);
    }

    // Draw selection indicator
    const int selY = (hue_ * (h - 1)) / 359;
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(0, selY, w, selY);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(0, selY - 2, w - 1, 4);
}

void HueSlider::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        updateFromPosition(event->pos());
    }
}

void HueSlider::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        updateFromPosition(event->pos());
    }
}

void HueSlider::updateFromPosition(const QPoint& pos)
{
    const int h = height();
    hue_ = std::clamp((pos.y() * 359) / std::max(1, h - 1), 0, 359);
    update();
    emit hueChanged(hue_);
}

// ============================================================================
// ColorChooserPanel Implementation
// ============================================================================

ColorChooserPanel::ColorChooserPanel(QWidget* parent) : QWidget(parent)
{
    setupUi();

    // Subscribe to color change events from other sources (e.g., eyedropper)
    colorChangedSub_ = EventBus::instance().subscribe<ColorChangedEvent>(
        [this](const ColorChangedEvent& event) {
            if (event.source != "color_panel") {
                setForegroundColor(event.color);
            }
        });
}

ColorChooserPanel::~ColorChooserPanel()
{
    EventBus::instance().unsubscribe(colorChangedSub_);
}

void ColorChooserPanel::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(8);

    auto* titleLabel = new QLabel("Colors", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    mainLayout_->addWidget(titleLabel);

    setupSwatchSection();
    setupColorPickerSection();
    setupRgbSection();
    setupHexSection();
    setupRecentColorsSection();

    mainLayout_->addStretch();

    // Initialize UI from default foreground color
    updateUiFromColor(foregroundColor_);
}

void ColorChooserPanel::setupSwatchSection()
{
    auto* swatchLayout = new QHBoxLayout();
    swatchLayout->setSpacing(4);

    // Foreground swatch
    foregroundSwatch_ = new QFrame(this);
    foregroundSwatch_->setFrameStyle(QFrame::Box | QFrame::Plain);
    foregroundSwatch_->setFixedSize(32, 32);
    foregroundSwatch_->setStyleSheet("background-color: #000000; border: 2px solid #666;");
    foregroundSwatch_->setCursor(Qt::PointingHandCursor);
    foregroundSwatch_->installEventFilter(this);
    swatchLayout->addWidget(foregroundSwatch_);

    // Swap button
    auto* swapButton = new QPushButton("⇄", this);
    swapButton->setFixedSize(24, 24);
    swapButton->setToolTip("Swap foreground and background colors (X)");
    connect(swapButton, &QPushButton::clicked, this, &ColorChooserPanel::onSwapColors);
    swatchLayout->addWidget(swapButton);

    // Background swatch
    backgroundSwatch_ = new QFrame(this);
    backgroundSwatch_->setFrameStyle(QFrame::Box | QFrame::Plain);
    backgroundSwatch_->setFixedSize(32, 32);
    backgroundSwatch_->setStyleSheet("background-color: #FFFFFF; border: 1px solid #999;");
    backgroundSwatch_->setCursor(Qt::PointingHandCursor);
    backgroundSwatch_->installEventFilter(this);
    swatchLayout->addWidget(backgroundSwatch_);

    swatchLayout->addStretch();

    // Labels
    auto* fgLabel = new QLabel("FG", this);
    fgLabel->setStyleSheet("font-size: 10px; color: #666;");
    auto* bgLabel = new QLabel("BG", this);
    bgLabel->setStyleSheet("font-size: 10px; color: #666;");

    auto* labelLayout = new QHBoxLayout();
    labelLayout->addWidget(fgLabel);
    labelLayout->addSpacing(28);
    labelLayout->addWidget(bgLabel);
    labelLayout->addStretch();

    mainLayout_->addLayout(swatchLayout);
    mainLayout_->addLayout(labelLayout);
}

void ColorChooserPanel::setupColorPickerSection()
{
    auto* pickerLayout = new QHBoxLayout();
    pickerLayout->setSpacing(4);

    colorSquare_ = new ColorSquare(this);
    connect(colorSquare_, &ColorSquare::colorChanged, this, &ColorChooserPanel::onColorSquareChanged);
    pickerLayout->addWidget(colorSquare_);

    hueSlider_ = new HueSlider(this);
    connect(hueSlider_, &HueSlider::hueChanged, this, &ColorChooserPanel::onHueChanged);
    pickerLayout->addWidget(hueSlider_);

    mainLayout_->addLayout(pickerLayout);
}

void ColorChooserPanel::setupRgbSection()
{
    auto* rgbLayout = new QGridLayout();
    rgbLayout->setSpacing(4);

    // Red
    auto* redLabel = new QLabel("R:", this);
    redSlider_ = new QSlider(Qt::Horizontal, this);
    redSlider_->setRange(0, 255);
    redSpinBox_ = new QSpinBox(this);
    redSpinBox_->setRange(0, 255);
    redSpinBox_->setFixedWidth(50);
    rgbLayout->addWidget(redLabel, 0, 0);
    rgbLayout->addWidget(redSlider_, 0, 1);
    rgbLayout->addWidget(redSpinBox_, 0, 2);

    // Green
    auto* greenLabel = new QLabel("G:", this);
    greenSlider_ = new QSlider(Qt::Horizontal, this);
    greenSlider_->setRange(0, 255);
    greenSpinBox_ = new QSpinBox(this);
    greenSpinBox_->setRange(0, 255);
    greenSpinBox_->setFixedWidth(50);
    rgbLayout->addWidget(greenLabel, 1, 0);
    rgbLayout->addWidget(greenSlider_, 1, 1);
    rgbLayout->addWidget(greenSpinBox_, 1, 2);

    // Blue
    auto* blueLabel = new QLabel("B:", this);
    blueSlider_ = new QSlider(Qt::Horizontal, this);
    blueSlider_->setRange(0, 255);
    blueSpinBox_ = new QSpinBox(this);
    blueSpinBox_->setRange(0, 255);
    blueSpinBox_->setFixedWidth(50);
    rgbLayout->addWidget(blueLabel, 2, 0);
    rgbLayout->addWidget(blueSlider_, 2, 1);
    rgbLayout->addWidget(blueSpinBox_, 2, 2);

    // Connect sliders and spinboxes
    connect(redSlider_, &QSlider::valueChanged, redSpinBox_, &QSpinBox::setValue);
    connect(redSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), redSlider_, &QSlider::setValue);
    connect(greenSlider_, &QSlider::valueChanged, greenSpinBox_, &QSpinBox::setValue);
    connect(greenSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), greenSlider_, &QSlider::setValue);
    connect(blueSlider_, &QSlider::valueChanged, blueSpinBox_, &QSpinBox::setValue);
    connect(blueSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), blueSlider_, &QSlider::setValue);

    // Connect to update handler
    connect(redSlider_, &QSlider::valueChanged, this, &ColorChooserPanel::onRgbSliderChanged);
    connect(greenSlider_, &QSlider::valueChanged, this, &ColorChooserPanel::onRgbSliderChanged);
    connect(blueSlider_, &QSlider::valueChanged, this, &ColorChooserPanel::onRgbSliderChanged);

    mainLayout_->addLayout(rgbLayout);
}

void ColorChooserPanel::setupHexSection()
{
    auto* hexLayout = new QHBoxLayout();
    hexLayout->setSpacing(4);

    auto* hexLabel = new QLabel("Hex:", this);
    hexLayout->addWidget(hexLabel);

    hexInput_ = new QLineEdit(this);
    hexInput_->setMaxLength(7);
    hexInput_->setPlaceholderText("#RRGGBB");
    hexInput_->setFixedWidth(80);

    // Validate hex input
    auto* validator = new QRegularExpressionValidator(
        QRegularExpression("^#?[0-9A-Fa-f]{0,6}$"), hexInput_);
    hexInput_->setValidator(validator);

    connect(hexInput_, &QLineEdit::editingFinished, this, &ColorChooserPanel::onHexInputFinished);
    hexLayout->addWidget(hexInput_);

    hexLayout->addStretch();

    mainLayout_->addLayout(hexLayout);
}

void ColorChooserPanel::setupRecentColorsSection()
{
    auto* recentLabel = new QLabel("Recent:", this);
    recentLabel->setStyleSheet("font-size: 10px; color: #666;");
    mainLayout_->addWidget(recentLabel);

    auto* recentLayout = new QHBoxLayout();
    recentLayout->setSpacing(2);

    recentSwatches_.reserve(kMaxRecentColors);
    for (std::size_t i = 0; i < kMaxRecentColors; ++i) {
        auto* swatch = new QFrame(this);
        swatch->setFrameStyle(QFrame::Box | QFrame::Plain);
        swatch->setFixedSize(20, 20);
        swatch->setStyleSheet("background-color: #808080; border: 1px solid #555;");
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setProperty("colorIndex", static_cast<int>(i));
        swatch->installEventFilter(this);
        recentSwatches_.push_back(swatch);
        recentLayout->addWidget(swatch);
    }

    recentLayout->addStretch();
    mainLayout_->addLayout(recentLayout);
}

void ColorChooserPanel::setForegroundColor(std::uint32_t color)
{
    if (foregroundColor_ == color) {
        return;
    }

    foregroundColor_ = color;
    addToRecentColors(color);

    if (editingForeground_) {
        updateUiFromColor(color);
    }

    // Update swatch
    const int r = static_cast<int>((color >> 24) & 0xFF);
    const int g = static_cast<int>((color >> 16) & 0xFF);
    const int b = static_cast<int>((color >> 8) & 0xFF);
    foregroundSwatch_->setStyleSheet(
        QString("background-color: rgb(%1,%2,%3); border: 2px solid #666;").arg(r).arg(g).arg(b));

    emit foregroundColorChanged(color);
}

void ColorChooserPanel::setBackgroundColor(std::uint32_t color)
{
    if (backgroundColor_ == color) {
        return;
    }

    backgroundColor_ = color;

    if (!editingForeground_) {
        updateUiFromColor(color);
    }

    // Update swatch
    const int r = static_cast<int>((color >> 24) & 0xFF);
    const int g = static_cast<int>((color >> 16) & 0xFF);
    const int b = static_cast<int>((color >> 8) & 0xFF);
    backgroundSwatch_->setStyleSheet(
        QString("background-color: rgb(%1,%2,%3); border: 1px solid #999;").arg(r).arg(g).arg(b));

    emit backgroundColorChanged(color);
}

void ColorChooserPanel::onColorSquareChanged(int saturation, int value)
{
    if (updatingUi_) {
        return;
    }

    currentSaturation_ = saturation;
    currentValue_ = value;

    int r = 0;
    int g = 0;
    int b = 0;
    hsvToRgb(currentHue_, saturation, value, r, g, b);

    updatingUi_ = true;
    redSlider_->setValue(r);
    greenSlider_->setValue(g);
    blueSlider_->setValue(b);
    hexInput_->setText(QString("#%1%2%3")
                           .arg(r, 2, 16, QChar('0'))
                           .arg(g, 2, 16, QChar('0'))
                           .arg(b, 2, 16, QChar('0'))
                           .toUpper());
    updatingUi_ = false;

    publishColorChange();
}

void ColorChooserPanel::onHueChanged(int hue)
{
    if (updatingUi_) {
        return;
    }

    currentHue_ = hue;
    colorSquare_->setHue(hue);

    int r = 0;
    int g = 0;
    int b = 0;
    hsvToRgb(hue, currentSaturation_, currentValue_, r, g, b);

    updatingUi_ = true;
    redSlider_->setValue(r);
    greenSlider_->setValue(g);
    blueSlider_->setValue(b);
    hexInput_->setText(QString("#%1%2%3")
                           .arg(r, 2, 16, QChar('0'))
                           .arg(g, 2, 16, QChar('0'))
                           .arg(b, 2, 16, QChar('0'))
                           .toUpper());
    updatingUi_ = false;

    publishColorChange();
}

void ColorChooserPanel::onRgbSliderChanged()
{
    if (updatingUi_) {
        return;
    }

    const int r = redSlider_->value();
    const int g = greenSlider_->value();
    const int b = blueSlider_->value();

    rgbToHsv(r, g, b, currentHue_, currentSaturation_, currentValue_);

    updatingUi_ = true;
    hueSlider_->setHue(currentHue_);
    colorSquare_->setHue(currentHue_);
    colorSquare_->setSaturation(currentSaturation_);
    colorSquare_->setValue(currentValue_);
    hexInput_->setText(QString("#%1%2%3")
                           .arg(r, 2, 16, QChar('0'))
                           .arg(g, 2, 16, QChar('0'))
                           .arg(b, 2, 16, QChar('0'))
                           .toUpper());
    updatingUi_ = false;

    publishColorChange();
}

void ColorChooserPanel::onHexInputFinished()
{
    if (updatingUi_) {
        return;
    }

    QString hex = hexInput_->text();
    if (hex.startsWith('#')) {
        hex = hex.mid(1);
    }

    if (hex.length() != 6) {
        return;
    }

    bool ok = false;
    const int value = hex.toInt(&ok, 16);
    if (!ok) {
        return;
    }

    const int r = (value >> 16) & 0xFF;
    const int g = (value >> 8) & 0xFF;
    const int b = value & 0xFF;

    updateFromRgb(r, g, b);
    publishColorChange();
}

void ColorChooserPanel::onSwapColors()
{
    const std::uint32_t temp = foregroundColor_;
    setForegroundColor(backgroundColor_);
    setBackgroundColor(temp);
    publishColorChange();
}

void ColorChooserPanel::onForegroundClicked()
{
    editingForeground_ = true;
    foregroundSwatch_->setStyleSheet(foregroundSwatch_->styleSheet().replace("1px", "2px"));
    updateUiFromColor(foregroundColor_);
}

void ColorChooserPanel::onBackgroundClicked()
{
    editingForeground_ = false;
    backgroundSwatch_->setStyleSheet(backgroundSwatch_->styleSheet().replace("1px", "2px"));
    updateUiFromColor(backgroundColor_);
}

void ColorChooserPanel::onRecentColorClicked()
{
    auto* swatch = qobject_cast<QFrame*>(sender());
    if (swatch == nullptr) {
        return;
    }

    const int index = swatch->property("colorIndex").toInt();
    if (index >= 0 && index < static_cast<int>(recentColors_.size())) {
        if (editingForeground_) {
            setForegroundColor(recentColors_[static_cast<std::size_t>(index)]);
        } else {
            setBackgroundColor(recentColors_[static_cast<std::size_t>(index)]);
        }
        publishColorChange();
    }
}

void ColorChooserPanel::updateFromHsv(int hue, int saturation, int value)
{
    currentHue_ = hue;
    currentSaturation_ = saturation;
    currentValue_ = value;

    int r = 0;
    int g = 0;
    int b = 0;
    hsvToRgb(hue, saturation, value, r, g, b);

    updatingUi_ = true;
    hueSlider_->setHue(hue);
    colorSquare_->setHue(hue);
    colorSquare_->setSaturation(saturation);
    colorSquare_->setValue(value);
    redSlider_->setValue(r);
    greenSlider_->setValue(g);
    blueSlider_->setValue(b);
    hexInput_->setText(QString("#%1%2%3")
                           .arg(r, 2, 16, QChar('0'))
                           .arg(g, 2, 16, QChar('0'))
                           .arg(b, 2, 16, QChar('0'))
                           .toUpper());
    updatingUi_ = false;
}

void ColorChooserPanel::updateFromRgb(int red, int green, int blue)
{
    rgbToHsv(red, green, blue, currentHue_, currentSaturation_, currentValue_);

    updatingUi_ = true;
    hueSlider_->setHue(currentHue_);
    colorSquare_->setHue(currentHue_);
    colorSquare_->setSaturation(currentSaturation_);
    colorSquare_->setValue(currentValue_);
    redSlider_->setValue(red);
    greenSlider_->setValue(green);
    blueSlider_->setValue(blue);
    hexInput_->setText(QString("#%1%2%3")
                           .arg(red, 2, 16, QChar('0'))
                           .arg(green, 2, 16, QChar('0'))
                           .arg(blue, 2, 16, QChar('0'))
                           .toUpper());
    updatingUi_ = false;
}

void ColorChooserPanel::updateUiFromColor(std::uint32_t color)
{
    const int r = static_cast<int>((color >> 24) & 0xFF);
    const int g = static_cast<int>((color >> 16) & 0xFF);
    const int b = static_cast<int>((color >> 8) & 0xFF);

    updateFromRgb(r, g, b);
}

void ColorChooserPanel::addToRecentColors(std::uint32_t color)
{
    // Don't add duplicates
    auto it = std::find(recentColors_.begin(), recentColors_.end(), color);
    if (it != recentColors_.end()) {
        recentColors_.erase(it);
    }

    // Add to front
    recentColors_.insert(recentColors_.begin(), color);

    // Trim to max size
    if (recentColors_.size() > kMaxRecentColors) {
        recentColors_.resize(kMaxRecentColors);
    }

    // Update swatches
    for (std::size_t i = 0; i < recentSwatches_.size(); ++i) {
        if (i < recentColors_.size()) {
            const std::uint32_t c = recentColors_[i];
            const int cr = static_cast<int>((c >> 24) & 0xFF);
            const int cg = static_cast<int>((c >> 16) & 0xFF);
            const int cb = static_cast<int>((c >> 8) & 0xFF);
            recentSwatches_[i]->setStyleSheet(
                QString("background-color: rgb(%1,%2,%3); border: 1px solid #555;")
                    .arg(cr)
                    .arg(cg)
                    .arg(cb));
        }
    }
}

void ColorChooserPanel::publishColorChange()
{
    const int r = redSlider_->value();
    const int g = greenSlider_->value();
    const int b = blueSlider_->value();

    const auto color = static_cast<std::uint32_t>((r << 24) | (g << 16) | (b << 8) | 0xFF);

    if (editingForeground_) {
        foregroundColor_ = color;
        addToRecentColors(color);

        foregroundSwatch_->setStyleSheet(
            QString("background-color: rgb(%1,%2,%3); border: 2px solid #666;").arg(r).arg(g).arg(b));

        // Publish event
        ColorChangedEvent event;
        event.color = color;
        event.source = "color_panel";
        EventBus::instance().publish(event);

        emit foregroundColorChanged(color);
    } else {
        backgroundColor_ = color;
        backgroundSwatch_->setStyleSheet(
            QString("background-color: rgb(%1,%2,%3); border: 1px solid #999;").arg(r).arg(g).arg(b));

        emit backgroundColorChanged(color);
    }
}

void ColorChooserPanel::hsvToRgb(int h, int s, int v, int& r, int& g, int& b)
{
    if (s == 0) {
        r = g = b = v;
        return;
    }

    const double hh = h / 60.0;
    const int i = static_cast<int>(hh);
    const double f = hh - i;
    const double p = v * (1.0 - s / 255.0);
    const double q = v * (1.0 - (s / 255.0) * f);
    const double t = v * (1.0 - (s / 255.0) * (1.0 - f));

    switch (i) {
        case 0:
            r = v;
            g = static_cast<int>(t);
            b = static_cast<int>(p);
            break;
        case 1:
            r = static_cast<int>(q);
            g = v;
            b = static_cast<int>(p);
            break;
        case 2:
            r = static_cast<int>(p);
            g = v;
            b = static_cast<int>(t);
            break;
        case 3:
            r = static_cast<int>(p);
            g = static_cast<int>(q);
            b = v;
            break;
        case 4:
            r = static_cast<int>(t);
            g = static_cast<int>(p);
            b = v;
            break;
        default:
            r = v;
            g = static_cast<int>(p);
            b = static_cast<int>(q);
            break;
    }
}

void ColorChooserPanel::rgbToHsv(int r, int g, int b, int& h, int& s, int& v)
{
    const int maxVal = std::max({r, g, b});
    const int minVal = std::min({r, g, b});
    const int delta = maxVal - minVal;

    v = maxVal;

    if (maxVal == 0) {
        s = 0;
        h = 0;
        return;
    }

    s = (delta * 255) / maxVal;

    if (delta == 0) {
        h = 0;
        return;
    }

    if (r == maxVal) {
        h = 60 * ((g - b) / static_cast<double>(delta));
    } else if (g == maxVal) {
        h = 60 * (2.0 + (b - r) / static_cast<double>(delta));
    } else {
        h = 60 * (4.0 + (r - g) / static_cast<double>(delta));
    }

    if (h < 0) {
        h += 360;
    }
}

}  // namespace gimp
