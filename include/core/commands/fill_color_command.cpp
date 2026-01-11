/**
 * @file fill_color_command.cpp
 * @brief Command stub to fill a document with a color.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#include "fill_color_command.h"

#include "../document.h"

namespace gimp {
FillColorCommand::FillColorCommand(std::shared_ptr<Document> document, std::uint32_t rgba)
    : document_{std::move(document)}, color_{rgba}
{
}

void FillColorCommand::apply()
{
    // TODO: implement fill operation once Document/TileStore exposes write API.
    (void)document_;
    (void)color_;
}

void FillColorCommand::undo()
{
    // TODO: implement undo once fill is implemented.
}
}  // namespace gimp
