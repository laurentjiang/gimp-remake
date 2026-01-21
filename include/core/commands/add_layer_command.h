/**
 * @file add_layer_command.h
 * @brief Command stub to add a layer to the document.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include "../command.h"

#include <memory>

namespace gimp {
class Document;
class Layer;

class AddLayerCommand : public Command {
  public:
    explicit AddLayerCommand(std::shared_ptr<Document> document);

    void apply() override;
    void undo() override;

  private:
    std::shared_ptr<Document> document_;
    std::shared_ptr<Layer> layer_;
};
}  // namespace gimp
