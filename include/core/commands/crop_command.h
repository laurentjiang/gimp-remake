/**
 * @file crop_command.h
 * @brief Command to crop the document canvas.
 * @author Aless Tosi
 * @date 2026-02-09
 */

#pragma once

#include "core/command.h"
#include "core/selection_manager.h"

#include <QPainterPath>
#include <QPoint>
#include <QRect>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

class Document;
class Layer;

/**
 * @brief Command to crop the canvas to a given bounds rectangle.
 */
class CropCommand : public Command {
  public:
    /**
     * @brief Constructs a crop command.
     * @param document Target document.
     * @param cropBounds Bounds to crop to, in canvas coordinates.
     */
    CropCommand(std::shared_ptr<Document> document, QRect cropBounds);

    ~CropCommand() override = default;

    /**
     * @brief Applies the crop operation.
     */
    void apply() override;

    /**
     * @brief Undoes the crop operation.
     */
    void undo() override;

  private:
    struct LayerSnapshot {
        std::shared_ptr<Layer> layer;
        std::vector<std::uint8_t> data;
    };

    void captureBeforeState();
    void restoreBeforeLayers();
    void restoreSelection(const QPainterPath& path, SelectionType type, const char* source);
    [[nodiscard]] QPainterPath buildAfterSelection() const;
    [[nodiscard]] QPoint computeOffset() const;
    void computeAnchor();

    std::shared_ptr<Document> document_;
    QRect cropBounds_;
    int targetWidth_ = 0;
    int targetHeight_ = 0;
    float anchorX_ = 0.0F;
    float anchorY_ = 0.0F;
    bool captured_ = false;
    bool valid_ = false;

    int beforeWidth_ = 0;
    int beforeHeight_ = 0;
    std::vector<LayerSnapshot> beforeLayers_;

    QPainterPath beforeSelection_;
    SelectionType beforeSelectionType_ = SelectionType::Unknown;
    QPainterPath afterSelection_;
    SelectionType afterSelectionType_ = SelectionType::Unknown;
};

}  // namespace gimp
