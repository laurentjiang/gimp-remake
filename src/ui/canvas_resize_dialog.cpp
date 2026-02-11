/**
 * @file canvas_resize_dialog.cpp
 * @brief Implementation of CanvasResizeDialog.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "ui/canvas_resize_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace gimp {

namespace {
constexpr int kAnchorGridSize = 3;
constexpr int kAnchorButtonSize = 32;
constexpr int kMaxCanvasSize = 100000;

const char* kAnchorLabels[kAnchorGridSize][kAnchorGridSize] = {
    {"TL", "TC", "TR"},
    {"CL", "CC", "CR"},
    {"BL", "BC", "BR"},
};
}  // namespace

CanvasResizeDialog::CanvasResizeDialog(int currentWidth, int currentHeight, QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    widthSpin_->setValue(currentWidth);
    heightSpin_->setValue(currentHeight);
    selectAnchor(1, 1);
    adjustSize();
    setFixedSize(sizeHint());
}

int CanvasResizeDialog::canvasWidth() const
{
    return widthSpin_ ? widthSpin_->value() : 0;
}

int CanvasResizeDialog::canvasHeight() const
{
    return heightSpin_ ? heightSpin_->value() : 0;
}

float CanvasResizeDialog::anchorX() const
{
    return static_cast<float>(anchorCol_) / 2.0F;
}

float CanvasResizeDialog::anchorY() const
{
    return static_cast<float>(anchorRow_) / 2.0F;
}

void CanvasResizeDialog::setupUi()
{
    setWindowTitle("Canvas Size");

    auto* mainLayout = new QVBoxLayout(this);

    auto* formLayout = new QFormLayout();
    widthSpin_ = new QSpinBox(this);
    widthSpin_->setRange(1, kMaxCanvasSize);
    widthSpin_->setSingleStep(10);

    heightSpin_ = new QSpinBox(this);
    heightSpin_->setRange(1, kMaxCanvasSize);
    heightSpin_->setSingleStep(10);

    formLayout->addRow("Width:", widthSpin_);
    formLayout->addRow("Height:", heightSpin_);
    mainLayout->addLayout(formLayout);

    auto* anchorLabel = new QLabel("Anchor", this);
    mainLayout->addWidget(anchorLabel);

    anchorGroup_ = new QButtonGroup(this);
    anchorGroup_->setExclusive(true);

    auto* anchorGrid = new QGridLayout();
    for (int row = 0; row < kAnchorGridSize; ++row) {
        for (int col = 0; col < kAnchorGridSize; ++col) {
            auto* button = new QToolButton(this);
            button->setText(kAnchorLabels[row][col]);
            button->setCheckable(true);
            button->setFixedSize(kAnchorButtonSize, kAnchorButtonSize);
            button->setStyleSheet(
                "QToolButton { background: #3c3c3c; border: 1px solid #555; }"
                "QToolButton:checked { background: #0078d4; border: 1px solid #005a9e; }"
                "QToolButton:hover { background: #4a4a4a; }");
            anchorGroup_->addButton(button, row * kAnchorGridSize + col);
            anchorGrid->addWidget(button, row, col);

            connect(button, &QToolButton::clicked, this, [this, row, col]() {
                selectAnchor(row, col);
            });
        }
    }
    mainLayout->addLayout(anchorGrid);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void CanvasResizeDialog::selectAnchor(int row, int col)
{
    anchorRow_ = row;
    anchorCol_ = col;

    if (anchorGroup_) {
        const int id = row * kAnchorGridSize + col;
        if (auto* button = anchorGroup_->button(id)) {
            button->setChecked(true);
        }
    }
}

}  // namespace gimp
