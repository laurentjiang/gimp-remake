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

#include <cstdint>
#include <string>
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
    /*! @brief Constructs a ColorSquare widget.
     *  @param parent The parent widget.
     */
    explicit ColorSquare(QWidget* parent = nullptr);

    /*! @brief Sets the current hue value.
     *  @param hue Hue value (0-359).
     */
    void setHue(int hue);

    /*! @brief Sets the saturation value.
     *  @param saturation Saturation value (0-255).
     */
    void setSaturation(int saturation);

    /*! @brief Sets the value (brightness).
     *  @param value Value/brightness (0-255).
     */
    void setValue(int value);

    /*! @brief Returns the current saturation.
     *  @return Saturation value (0-255).
     */
    [[nodiscard]] int saturation() const { return saturation_; }

    /*! @brief Returns the current value (brightness).
     *  @return Value/brightness (0-255).
     */
    [[nodiscard]] int value() const { return value_; }

  signals:
    /*! @brief Emitted when saturation or value changes.
     *  @param saturation New saturation value.
     *  @param value New value/brightness.
     */
    void colorChanged(int saturation, int value);

  protected:
    /*! @brief Handles paint events. */
    void paintEvent(QPaintEvent* event) override;

    /*! @brief Handles mouse press events. */
    void mousePressEvent(QMouseEvent* event) override;

    /*! @brief Handles mouse move events. */
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
    /*! @brief Constructs a HueSlider widget.
     *  @param parent The parent widget.
     */
    explicit HueSlider(QWidget* parent = nullptr);

    /*! @brief Sets the current hue value.
     *  @param hue Hue value (0-359).
     */
    void setHue(int hue);

    /*! @brief Returns the current hue.
     *  @return Hue value (0-359).
     */
    [[nodiscard]] int hue() const { return hue_; }

  signals:
    /*! @brief Emitted when the hue value changes.
     *  @param hue New hue value.
     */
    void hueChanged(int hue);

  protected:
    /*! @brief Handles paint events. */
    void paintEvent(QPaintEvent* event) override;

    /*! @brief Handles mouse press events. */
    void mousePressEvent(QMouseEvent* event) override;

    /*! @brief Handles mouse move events. */
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
    /*! @brief Constructs a ColorChooserPanel widget.
     *  @param parent The parent widget.
     */
    explicit ColorChooserPanel(QWidget* parent = nullptr);

    /*! @brief Destructor. */
    ~ColorChooserPanel() override;

    /*! @brief Sets the foreground color.
     *  @param color Color in RGBA format (0xRRGGBBAA).
     */
    void setForegroundColor(std::uint32_t color);

    /*! @brief Sets the background color.
     *  @param color Color in RGBA format (0xRRGGBBAA).
     */
    void setBackgroundColor(std::uint32_t color);

    /*! @brief Returns the current foreground color.
     *  @return Foreground color in RGBA format.
     */
    [[nodiscard]] std::uint32_t foregroundColor() const { return foregroundColor_; }

    /*! @brief Returns the current background color.
     *  @return Background color in RGBA format.
     */
    [[nodiscard]] std::uint32_t backgroundColor() const { return backgroundColor_; }

    /*! @brief Swaps the foreground and background colors. */
    void swapColors();

    /*! @brief Resets colors to defaults (black foreground, white background). */
    void resetToDefaults();

  protected:
    /*! @brief Event filter for handling swatch clicks.
     *  @param watched The watched object.
     *  @param event The event.
     *  @return True if event was handled.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

  signals:
    /*! @brief Emitted when the foreground color changes.
     *  @param color New foreground color in RGBA format.
     */
    void foregroundColorChanged(std::uint32_t color);

    /*! @brief Emitted when the background color changes.
     *  @param color New background color in RGBA format.
     */
    void backgroundColorChanged(std::uint32_t color);

  private slots:
    void onColorSquareChanged(int saturation, int value);
    void onHueChanged(int hue);
    void onRgbSliderChanged();
    void onHexInputFinished();
    void onSwapColors();
    void onForegroundClicked();
    void onBackgroundClicked();

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
    /*! @brief Converts HSV to RGB color values.
     *  @param h Hue (0-359).
     *  @param s Saturation (0-255).
     *  @param v Value/brightness (0-255).
     *  @param r Output red component (0-255).
     *  @param g Output green component (0-255).
     *  @param b Output blue component (0-255).
     */
    static void hsvToRgb(int h, int s, int v, int& r, int& g, int& b);

    /*! @brief Converts RGB to HSV color values.
     *  @param r Red component (0-255).
     *  @param g Green component (0-255).
     *  @param b Blue component (0-255).
     *  @param h Output hue (0-359).
     *  @param s Output saturation (0-255).
     *  @param v Output value/brightness (0-255).
     */
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
    EventBus::SubscriptionId colorUsedSub_ = 0;

    // Flag to prevent recursive updates
    bool updatingUi_ = false;
};

}  // namespace gimp
