/**
 * @file new_document_dialog.cpp
 * @brief Implementation of the NewDocumentDialog.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#include "ui/new_document_dialog.h"

#include "ui/theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVariant>
#include <QVBoxLayout>

#include <algorithm>

namespace gimp {

namespace {

QSize sizeFromItem(const QComboBox* combo, int index)
{
    const QVariant data = combo->itemData(index);
    if (!data.isValid()) {
        return {};
    }
    return data.toSize();
}

QString sizeLabel(const QSize& size)
{
    return QString("%1 x %2").arg(size.width()).arg(size.height());
}

}  // namespace

NewDocumentDialog::NewDocumentDialog(std::uint32_t backgroundColor, QWidget* parent)
    : QDialog(parent), backgroundColor_(backgroundColor)
{
    setWindowTitle("New Project");
    setModal(true);

    setupUi();
    populatePresets();
    populateRecent();
    updateBackgroundSwatch();
}

NewDocumentSettings NewDocumentDialog::settings() const
{
    NewDocumentSettings result;
    result.width = widthSpin_->value();
    result.height = heightSpin_->value();
    result.dpi = dpiSpin_->value();
    result.backgroundColor = backgroundColor_;

    switch (backgroundCombo_->currentIndex()) {
        case 1:
            result.backgroundFill = BackgroundFill::Transparent;
            break;
        case 2:
            result.backgroundFill = BackgroundFill::BackgroundColor;
            break;
        default:
            result.backgroundFill = BackgroundFill::White;
            break;
    }

    return result;
}

void NewDocumentDialog::addRecentSize(const QSize& size)
{
    if (size.width() <= 0 || size.height() <= 0) {
        return;
    }

    auto& storage = recentSizesStorage();
    storage.erase(std::remove(storage.begin(), storage.end(), size), storage.end());
    storage.insert(storage.begin(), size);

    const std::size_t kMaxRecent = 6;
    if (storage.size() > kMaxRecent) {
        storage.resize(kMaxRecent);
    }
}

const std::vector<QSize>& NewDocumentDialog::recentSizes()
{
    return recentSizesStorage();
}

void NewDocumentDialog::onPresetChanged(int index)
{
    if (updatingSize_) {
        return;
    }

    const QSize size = sizeFromItem(presetCombo_, index);
    if (!size.isValid()) {
        return;
    }

    updatingSize_ = true;
    widthSpin_->setValue(size.width());
    heightSpin_->setValue(size.height());
    if (recentCombo_->isEnabled()) {
        recentCombo_->setCurrentIndex(0);
    }
    updatingSize_ = false;
}

void NewDocumentDialog::onRecentChanged(int index)
{
    if (updatingSize_) {
        return;
    }

    const QSize size = sizeFromItem(recentCombo_, index);
    if (!size.isValid()) {
        return;
    }

    updatingSize_ = true;
    widthSpin_->setValue(size.width());
    heightSpin_->setValue(size.height());
    presetCombo_->setCurrentIndex(0);
    updatingSize_ = false;
}

void NewDocumentDialog::onSizeEdited()
{
    if (updatingSize_) {
        return;
    }

    updatingSize_ = true;
    presetCombo_->setCurrentIndex(0);
    if (recentCombo_->isEnabled()) {
        recentCombo_->setCurrentIndex(0);
    }
    updatingSize_ = false;
}

void NewDocumentDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft);

    presetCombo_ = new QComboBox(this);
    formLayout->addRow("Preset", presetCombo_);

    recentCombo_ = new QComboBox(this);
    formLayout->addRow("Recent", recentCombo_);

    widthSpin_ = new QSpinBox(this);
    widthSpin_->setRange(1, 20000);
    widthSpin_->setValue(800);
    widthSpin_->setSuffix(" px");
    formLayout->addRow("Width", widthSpin_);

    heightSpin_ = new QSpinBox(this);
    heightSpin_->setRange(1, 20000);
    heightSpin_->setValue(600);
    heightSpin_->setSuffix(" px");
    formLayout->addRow("Height", heightSpin_);

    dpiSpin_ = new QDoubleSpinBox(this);
    dpiSpin_->setRange(1.0, 1200.0);
    dpiSpin_->setDecimals(1);
    dpiSpin_->setSingleStep(1.0);
    dpiSpin_->setValue(72.0);
    dpiSpin_->setSuffix(" dpi");
    formLayout->addRow("Resolution", dpiSpin_);

    backgroundCombo_ = new QComboBox(this);
    backgroundCombo_->addItem("White");
    backgroundCombo_->addItem("Transparent");
    backgroundCombo_->addItem("Background Color");

    backgroundSwatch_ = new QFrame(this);
    backgroundSwatch_->setFixedSize(20, 20);
    backgroundSwatch_->setFrameShape(QFrame::Box);
    backgroundSwatch_->setLineWidth(1);

    auto* backgroundRow = new QHBoxLayout();
    backgroundRow->addWidget(backgroundCombo_);
    backgroundRow->addWidget(backgroundSwatch_);
    backgroundRow->addStretch();

    auto* backgroundContainer = new QWidget(this);
    backgroundContainer->setLayout(backgroundRow);
    formLayout->addRow("Background", backgroundContainer);

    mainLayout->addLayout(formLayout);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    connect(presetCombo_, &QComboBox::currentIndexChanged, this, &NewDocumentDialog::onPresetChanged);
    connect(recentCombo_, &QComboBox::currentIndexChanged, this, &NewDocumentDialog::onRecentChanged);
    connect(widthSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &NewDocumentDialog::onSizeEdited);
    connect(heightSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &NewDocumentDialog::onSizeEdited);
}

void NewDocumentDialog::populatePresets()
{
    presetCombo_->clear();
    presetCombo_->addItem("Custom", QSize());
    presetCombo_->addItem("720p (1280 x 720)", QSize(1280, 720));
    presetCombo_->addItem("1080p (1920 x 1080)", QSize(1920, 1080));
    presetCombo_->addItem("Square 512", QSize(512, 512));
    presetCombo_->addItem("Square 1024", QSize(1024, 1024));
}

void NewDocumentDialog::populateRecent()
{
    recentCombo_->clear();

    const auto& recent = recentSizesStorage();
    if (recent.empty()) {
        recentCombo_->addItem("None", QSize());
        recentCombo_->setEnabled(false);
        return;
    }

    recentCombo_->setEnabled(true);
    recentCombo_->addItem("Select recent", QSize());

    for (const auto& size : recent) {
        recentCombo_->addItem(sizeLabel(size), size);
    }
}

void NewDocumentDialog::updateBackgroundSwatch()
{
    const int red = static_cast<int>((backgroundColor_ >> 24) & 0xFF);
    const int green = static_cast<int>((backgroundColor_ >> 16) & 0xFF);
    const int blue = static_cast<int>((backgroundColor_ >> 8) & 0xFF);

    backgroundSwatch_->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); border: 1px solid %4;")
            .arg(red)
            .arg(green)
            .arg(blue)
            .arg(Theme::toHex(Theme::kBorderLight)));
}

std::vector<QSize>& NewDocumentDialog::recentSizesStorage()
{
    static std::vector<QSize> recent;
    return recent;
}

}  // namespace gimp
