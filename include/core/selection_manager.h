/**
 * @file selection_manager.h
 * @brief Singleton manager for document selection paths.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#pragma once

#include "core/document.h"

#include <QPainterPath>

#include <memory>

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
 * @brief Selection shape type for optimization hints.
 */
enum class SelectionType {
    Unknown,    ///< Complex or combined selection.
    Rectangle,  ///< Axis-aligned rectangle.
    Ellipse     ///< Axis-aligned ellipse.
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
    /**
     * @brief Returns the singleton instance of the SelectionManager.
     */
    static SelectionManager& instance()
    {
        static SelectionManager manager;
        return manager;
    }

    /**
     * @brief Sets the active document for selection storage.
     * @param document The active document.
     */
    void setDocument(const std::shared_ptr<Document>& document)
    {
        document_ = document;
        if (document) {
            selection_ = document->selectionPath();
        } else {
            selection_ = QPainterPath();
        }
        preview_ = QPainterPath();
        previewMode_ = SelectionMode::Replace;
    }

    /**
     * @brief Clears the selection and preview.
     */
    void clear()
    {
        selection_ = QPainterPath();
        preview_ = QPainterPath();
        previewMode_ = SelectionMode::Replace;
        selectionType_ = SelectionType::Unknown;
        syncSelectionToDocument();
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
     * @brief Returns the selection type for optimization hints.
     */
    [[nodiscard]] SelectionType selectionType() const { return selectionType_; }

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
     * @param path The selection path to apply.
     * @param mode How to combine with existing selection.
     * @param type Optional hint about the selection shape for optimization.
     */
    void applySelection(const QPainterPath& path,
                        SelectionMode mode,
                        SelectionType type = SelectionType::Unknown)
    {
        if (path.isEmpty()) {
            return;
        }

        switch (mode) {
            case SelectionMode::Replace:
                selection_ = path;
                selectionType_ = type;
                break;
            case SelectionMode::Add:
                selection_ = selection_.isEmpty() ? path : selection_.united(path);
                selectionType_ = SelectionType::Unknown;  // Combined selection
                break;
            case SelectionMode::Subtract:
                selection_ = selection_.isEmpty() ? QPainterPath() : selection_.subtracted(path);
                selectionType_ = SelectionType::Unknown;  // Combined selection
                break;
        }

        syncSelectionToDocument();
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

    void syncSelectionToDocument()
    {
        if (auto document = document_.lock()) {
            document->setSelectionPath(selection_);
        }
    }

    QPainterPath selection_;
    QPainterPath preview_;
    SelectionMode previewMode_ = SelectionMode::Replace;
    SelectionType selectionType_ = SelectionType::Unknown;
    std::weak_ptr<Document> document_;
};

}  // namespace gimp
