/**
 * @file color_chooser_panel.h
 * @brief Color chooser panel for selecting foreground and background colors.
 * @author Laurent Jiang
 * @date 2026-01-29
 */

#pragma once

#include "core/event_bus.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <array>
#include <cstdint>
#include <vector>

namespace gimp {

/**
 * @brief Custom widget for the color gradient square (saturation/brightness).
 *
 * Displays a gradient from white to the current hue (horizontal) and
 * from the hue to black (vertical). Users can click/drag to select color.
 */
class ColorSquare : public QWidget {
    Q_OBJECT

  public:
    explicit ColorSquare(QWidget* parent = nullptr);

    void setHue(int hue);
    void setSaturation(int saturation);
    void setValue(int value);

    [[nodiscard]] int saturation() const { return saturation_; }
    [[nodiscard]] int value() const { return value_; }

  signals:
    void colorChanged(int saturation, int value);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    void updateFromPosition(const QPoint& pos);

    int hue_ = 0;
    int saturation_ = 255;
    int value_ = 255;
};

/**
 * @brief Custom widget for the hue slider bar.
 *
 * Displays a vertical gradient of all hues (0-359).
 */
class HueSlider : public QWidget {
    Q_OBJECT

  public:
    explicit HueSlider(QWidget* parent = nullptr);

    void setHue(int hue);
    [[nodiscard]] int hue() const { return hue_; }

  signals:
    void hueChanged(int hue);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    void updateFromPosition(const QPoint& pos);

    int hue_ = 0;
};

/**
 * @brief Panel for selecting foreground and background colors.
 *
 * Features:
 * - Color square for saturation/brightness selection
 * - Hue slider
 * - RGB sliders with numeric inputs
 * - Hex code input
 * - Foreground/background swatches with swap button
 * - Recent colors history
 */
class ColorChooserPanel : public QWidget {
    Q_OBJECT

  public:
    explicit ColorChooserPanel(QWidget* parent = nullptr);
    ~ColorChooserPanel() override;

    void setForegroundColor(std::uint32_t color);
    void setBackgroundColor(std::uint32_t color);

    [[nodiscard]] std::uint32_t foregroundColor() const { return foregroundColor_; }
    [[nodiscard]] std::uint32_t backgroundColor() const { return backgroundColor_; }

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

  signals:
    void foregroundColorChanged(std::uint32_t color);
    void backgroundColorChanged(std::uint32_t color);

  private slots:
    void onColorSquareChanged(int saturation, int value);
    void onHueChanged(int hue);
    void onRgbSliderChanged();
    void onHexInputFinished();
    void onSwapColors();
    void onForegroundClicked();
    void onBackgroundClicked();
    void onRecentColorClicked();

  private:
    void setupUi();
    void setupColorPickerSection();
    void setupRgbSection();
    void setupHexSection();
    void setupSwatchSection();
    void setupRecentColorsSection();

    void updateFromHsv(int hue, int saturation, int value);
    void updateFromRgb(int red, int green, int blue);
    void updateUiFromColor(std::uint32_t color);
    void addToRecentColors(std::uint32_t color);
    void publishColorChange();

  public:
    // Color conversion utilities (public for use by ColorSquare/HueSlider)
    static void hsvToRgb(int h, int s, int v, int& r, int& g, int& b);
    static void rgbToHsv(int r, int g, int b, int& h, int& s, int& v);

  private:

    QVBoxLayout* mainLayout_ = nullptr;

    // Color picker section
    ColorSquare* colorSquare_ = nullptr;
    HueSlider* hueSlider_ = nullptr;

    // RGB sliders
    QSlider* redSlider_ = nullptr;
    QSlider* greenSlider_ = nullptr;
    QSlider* blueSlider_ = nullptr;
    QSpinBox* redSpinBox_ = nullptr;
    QSpinBox* greenSpinBox_ = nullptr;
    QSpinBox* blueSpinBox_ = nullptr;

    // Hex input
    QLineEdit* hexInput_ = nullptr;

    // Color swatches
    QFrame* foregroundSwatch_ = nullptr;
    QFrame* backgroundSwatch_ = nullptr;

    // Recent colors
    std::vector<QFrame*> recentSwatches_;
    std::vector<std::uint32_t> recentColors_;
    static constexpr std::size_t kMaxRecentColors = 8;

    // Current colors
    std::uint32_t foregroundColor_ = 0x000000FF;  // Black with full alpha
    std::uint32_t backgroundColor_ = 0xFFFFFFFF;  // White with full alpha
    bool editingForeground_ = true;

    // Current HSV values
    int currentHue_ = 0;
    int currentSaturation_ = 0;
    int currentValue_ = 0;

    // Event subscriptions
    EventBus::SubscriptionId colorChangedSub_ = 0;

    // Flag to prevent recursive updates
    bool updatingUi_ = false;
};

}  // namespace gimp
