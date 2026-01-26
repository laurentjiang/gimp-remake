/**
 * @file add_layer_command.cpp
 * @brief Command stub to add a layer to the document.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#include "core/commands/add_layer_command.h"

#include "core/document.h"

namespace gimp {
AddLayerCommand::AddLayerCommand(std::shared_ptr<Document> document)
    : document_{std::move(document)}
{
}

void AddLayerCommand::apply()
{
    if (!document_) {
        return;
    }

    if (!layer_) {
        layer_ = document_->add_layer();
    }
}

void AddLayerCommand::undo()
{
    if (document_ && layer_) {
        document_->remove_layer(layer_);
    }
}
}  // namespace gimp
