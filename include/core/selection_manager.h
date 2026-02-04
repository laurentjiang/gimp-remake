/**
 * @file selection_manager.h
 * @brief Singleton manager for document selection paths.
 * @author Laurent Jiang
 * @date 2026-02-04
 */

#pragma once

#include <QPainterPath>

namespace gimp {

/**
 * @brief Selection combination mode.
 */
enum class SelectionMode {
    Replace,  ///< Replace existing selection.
    Add,      ///< Add to existing selection.
    Subtract  ///< Subtract from existing selection.
};

/**
 * @brief Central manager for selection state.
 *
 * Stores the committed selection and a preview path during interactive
 * selection creation. The preview is combined with the committed selection
 * for display, but only applied on commit.
 */
class SelectionManager {
  public:
    static SelectionManager& instance()
    {
        static SelectionManager manager;
        return manager;
    }

    /**
     * @brief Clears the selection and preview.
     */
    void clear()
    {
        selection_ = QPainterPath();
        preview_ = QPainterPath();
        previewMode_ = SelectionMode::Replace;
    }

    /**
     * @brief Returns true if a committed selection exists.
     */
    [[nodiscard]] bool hasSelection() const { return !selection_.isEmpty(); }

    /**
     * @brief Returns true if a preview selection exists.
     */
    [[nodiscard]] bool hasPreview() const { return !preview_.isEmpty(); }

    /**
     * @brief Returns the committed selection path.
     */
    [[nodiscard]] const QPainterPath& selectionPath() const { return selection_; }

    /**
     * @brief Returns the preview selection path.
     */
    [[nodiscard]] const QPainterPath& previewPath() const { return preview_; }

    /**
     * @brief Sets the preview selection path and mode.
     */
    void setPreview(const QPainterPath& path, SelectionMode mode)
    {
        preview_ = path;
        previewMode_ = mode;
    }

    /**
     * @brief Clears the preview selection path.
     */
    void clearPreview()
    {
        preview_ = QPainterPath();
        previewMode_ = SelectionMode::Replace;
    }

    /**
     * @brief Returns the preview mode.
     */
    [[nodiscard]] SelectionMode previewMode() const { return previewMode_; }

    /**
     * @brief Applies a path to the committed selection using the given mode.
     */
    void applySelection(const QPainterPath& path, SelectionMode mode)
    {
        if (path.isEmpty()) {
            return;
        }

        switch (mode) {
            case SelectionMode::Replace:
                selection_ = path;
                break;
            case SelectionMode::Add:
                selection_ = selection_.isEmpty() ? path : selection_.united(path);
                break;
            case SelectionMode::Subtract:
                selection_ = selection_.isEmpty() ? QPainterPath() : selection_.subtracted(path);
                break;
        }
    }

    /**
     * @brief Returns the selection path used for display (combined with preview).
     */
    [[nodiscard]] QPainterPath displayPath() const
    {
        if (preview_.isEmpty()) {
            return selection_;
        }

        switch (previewMode_) {
            case SelectionMode::Replace:
                return preview_;
            case SelectionMode::Add:
                return selection_.united(preview_);
            case SelectionMode::Subtract:
                return selection_.subtracted(preview_);
        }
        return preview_;
    }

  private:
    SelectionManager() = default;

    QPainterPath selection_;
    QPainterPath preview_;
    SelectionMode previewMode_ = SelectionMode::Replace;
};

}  // namespace gimp
