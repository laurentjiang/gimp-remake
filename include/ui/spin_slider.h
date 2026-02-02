/**
 * @file spin_slider.h
 * @brief GIMP-style combined slider/spinbox widget.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#pragma once

#include <QLineEdit>
#include <QMouseEvent>
#include <QWidget>

namespace gimp {

/**
 * @brief Combined slider and spinbox widget, similar to GIMP's GimpSpinScale.
 *
 * Displays a slider bar with an embedded text input. Users can:
 * - Drag the slider to change values
 * - Click to edit the numeric value directly
 * - Scroll wheel to increment/decrement
 */
class SpinSlider : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructs a SpinSlider.
     * @param parent Parent widget.
     */
    explicit SpinSlider(QWidget* parent = nullptr);

    /*! @brief Sets the minimum value. */
    void setMinimum(double min);
    /*! @brief Sets the maximum value. */
    void setMaximum(double max);
    /*! @brief Sets the range. */
    void setRange(double min, double max);
    /*! @brief Sets the current value. */
    void setValue(double value);
    /*! @brief Sets the suffix displayed after the value (e.g., " px", "%"). */
    void setSuffix(const QString& suffix);
    /*! @brief Sets the number of decimal places. */
    void setDecimals(int decimals);
    /*! @brief Sets the single step increment. */
    void setSingleStep(double step);
    /*! @brief Sets the label text shown before the value. */
    void setLabel(const QString& label);

    /*! @brief Returns the current value. */
    [[nodiscard]] double value() const { return value_; }
    /*! @brief Returns the minimum value. */
    [[nodiscard]] double minimum() const { return min_; }
    /*! @brief Returns the maximum value. */
    [[nodiscard]] double maximum() const { return max_; }

  signals:
    /*! @brief Emitted when the value changes. */
    void valueChanged(double value);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

  private slots:
    void onEditFinished();

  private:
    void updateValueFromPosition(int x);
    void showEditor();
    void hideEditor();
    [[nodiscard]] QString formatValue() const;

    double min_ = 0.0;
    double max_ = 100.0;
    double value_ = 50.0;
    double step_ = 1.0;
    int decimals_ = 0;
    QString suffix_;
    QString label_;

    bool dragging_ = false;
    QLineEdit* editor_ = nullptr;
};

}  // namespace gimp

