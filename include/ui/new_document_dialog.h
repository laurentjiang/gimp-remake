/**
 * @file new_document_dialog.h
 * @brief Dialog for creating new documents with custom settings.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QSize>
#include <QSpinBox>

#include <cstdint>
#include <vector>

namespace gimp {

/**
 * @brief Background fill options for new documents.
 */
enum class BackgroundFill {
    White,
    Transparent,
    BackgroundColor
};

/**
 * @brief Settings selected in the New Document dialog.
 */
struct NewDocumentSettings {
    int width = 800;
    int height = 600;
    double dpi = 72.0;
    BackgroundFill backgroundFill = BackgroundFill::White;
    std::uint32_t backgroundColor = 0xFFFFFFFF;
};

/**
 * @brief Dialog for creating new documents with custom dimensions and settings.
 */
class NewDocumentDialog : public QDialog {
    Q_OBJECT

  public:
    explicit NewDocumentDialog(std::uint32_t backgroundColor, QWidget* parent = nullptr);

    [[nodiscard]] NewDocumentSettings settings() const;

    static void addRecentSize(const QSize& size);
    static const std::vector<QSize>& recentSizes();

  private slots:
    void onPresetChanged(int index);
    void onRecentChanged(int index);
    void onSizeEdited();

  private:
    void setupUi();
    void populatePresets();
    void populateRecent();
    void updateBackgroundSwatch();

    static std::vector<QSize>& recentSizesStorage();

    std::uint32_t backgroundColor_ = 0xFFFFFFFF;
    bool updatingSize_ = false;

    QComboBox* presetCombo_ = nullptr;
    QComboBox* recentCombo_ = nullptr;
    QSpinBox* widthSpin_ = nullptr;
    QSpinBox* heightSpin_ = nullptr;
    QDoubleSpinBox* dpiSpin_ = nullptr;
    QComboBox* backgroundCombo_ = nullptr;
    QFrame* backgroundSwatch_ = nullptr;
};

}  // namespace gimp
