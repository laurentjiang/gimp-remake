/**
 * @file canvas_resize_dialog.h
 * @brief Dialog for resizing the document canvas.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QSpinBox>

namespace gimp {

/**
 * @brief Dialog for adjusting canvas dimensions and anchor.
 */
class CanvasResizeDialog : public QDialog {
  public:
    /**
     * @brief Constructs the dialog with the current canvas size.
     * @param currentWidth Current canvas width.
     * @param currentHeight Current canvas height.
     * @param parent Optional parent widget.
     */
    explicit CanvasResizeDialog(int currentWidth, int currentHeight, QWidget* parent = nullptr);

    /**
     * @brief Returns the requested canvas width.
     */
    [[nodiscard]] int canvasWidth() const;

    /**
     * @brief Returns the requested canvas height.
     */
    [[nodiscard]] int canvasHeight() const;

    /**
     * @brief Returns the normalized horizontal anchor (0.0 = left, 1.0 = right).
     */
    [[nodiscard]] float anchorX() const;

    /**
     * @brief Returns the normalized vertical anchor (0.0 = top, 1.0 = bottom).
     */
    [[nodiscard]] float anchorY() const;

  private:
    void setupUi();
    void selectAnchor(int row, int col);

    QSpinBox* widthSpin_ = nullptr;
    QSpinBox* heightSpin_ = nullptr;
    QButtonGroup* anchorGroup_ = nullptr;
    int anchorRow_ = 1;
    int anchorCol_ = 1;
};

}  // namespace gimp
