/**
 * @file document.h
 * @brief Minimal document interface for the image model.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

#include <memory>
#include <vector>
#include "layer_stack.h"

namespace gimp {
class Layer;
class TileStore;

class Document {
  public:
    virtual ~Document() = default;

    virtual std::shared_ptr<Layer> add_layer() = 0;
    virtual void remove_layer(const std::shared_ptr<Layer>& layer) = 0;
    virtual const LayerStack& layers() const = 0;
    virtual TileStore& tile_store() = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
};
}  // namespace gimp
