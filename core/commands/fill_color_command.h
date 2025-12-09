/**
 * @file fill_color_command.h
 * @brief Command stub to fill a document with a color.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <cstdint>
#include <memory>
#include "../command.h"

namespace gimp {
class Document;

class FillColorCommand : public Command {
  public:
    FillColorCommand(std::shared_ptr<Document> document, std::uint32_t rgba);

    void apply() override;
    void undo() override;

  private:
    std::shared_ptr<Document> document_;
    std::uint32_t color_;
};
}  // namespace gimp
