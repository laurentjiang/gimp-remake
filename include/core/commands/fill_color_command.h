/**
 * @file fill_color_command.h
 * @brief Command stub to fill a document with a color.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include "core/command.h"

#include <cstdint>
#include <memory>

namespace gimp {
class Document;

/*!
 * @class FillColorCommand
 * @brief Command to fill a document with a solid color (undoable).
 */
class FillColorCommand : public Command {
  public:
    /*!
     * @brief Constructs the command with target document and fill color.
     * @param document The document to fill.
     * @param rgba The fill color in RGBA format (0xRRGGBBAA).
     */
    FillColorCommand(std::shared_ptr<Document> document, std::uint32_t rgba);

    void apply() override;
    void undo() override;

  private:
    std::shared_ptr<Document> document_;
    std::uint32_t color_;
};
}  // namespace gimp
