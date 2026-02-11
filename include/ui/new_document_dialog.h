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
  int width = 800;  ///< Document width in pixels.
  int height = 600;  ///< Document height in pixels.
  double dpi = 72.0;  ///< Resolution in DPI.
  BackgroundFill backgroundFill = BackgroundFill::White;  ///< Background fill option.
  std::uint32_t backgroundColor = 0xFFFFFFFF;  ///< Background color in RGBA.
};

/**
 * @brief Dialog for creating new documents with custom dimensions and settings.
 */
class NewDocumentDialog : public QDialog {
    Q_OBJECT

  public:
    /*! @brief Constructs the dialog.
     *  @param backgroundColor Initial background color in RGBA format.
     *  @param parent Optional parent widget.
     */
    explicit NewDocumentDialog(std::uint32_t backgroundColor, QWidget* parent = nullptr);

    /*! @brief Returns the settings selected by the user.
     *  @return Selected document settings.
     */
    [[nodiscard]] NewDocumentSettings settings() const;

    /*! @brief Adds a size to the recent sizes list.
     *  @param size Size to add.
     */
    static void addRecentSize(const QSize& size);

    /*! @brief Returns the list of recent sizes.
     *  @return Vector of recent sizes.
     */
    static const std::vector<QSize>& recentSizes();

  private slots:
    /*! @brief Updates width/height when a preset is chosen.
     *  @param index Selected preset index.
     */
    void onPresetChanged(int index);
    /*! @brief Updates width/height when a recent size is chosen.
     *  @param index Selected recent size index.
     */
    void onRecentChanged(int index);
    /*! @brief Clears preset selection when size values are edited. */
    void onSizeEdited();

  private:
    /*! @brief Builds the dialog layout and widgets. */
    void setupUi();
    /*! @brief Populates the preset sizes list. */
    void populatePresets();
    /*! @brief Populates the recent sizes list. */
    void populateRecent();
    /*! @brief Updates the background color swatch. */
    void updateBackgroundSwatch();

    /*! @brief Returns the static recent size storage. */
    static std::vector<QSize>& recentSizesStorage();

    std::uint32_t backgroundColor_ = 0xFFFFFFFF;  ///< Background color in RGBA.
    bool updatingSize_ = false;  ///< Guards against recursive size updates.

    QComboBox* presetCombo_ = nullptr;  ///< Preset size selector.
    QComboBox* recentCombo_ = nullptr;  ///< Recent size selector.
    QSpinBox* widthSpin_ = nullptr;  ///< Width input.
    QSpinBox* heightSpin_ = nullptr;  ///< Height input.
    QDoubleSpinBox* dpiSpin_ = nullptr;  ///< DPI input.
    QComboBox* backgroundCombo_ = nullptr;  ///< Background fill selector.
    QFrame* backgroundSwatch_ = nullptr;  ///< Background color preview.
};

}  // namespace gimp
