/**
 * @file layer_stack.h
 * @brief Manages the stack of layers in a document.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include <memory>
#include <vector>
#include "layer.h"

namespace gimp {

class LayerStack {
  public:
    using iterator = std::vector<std::shared_ptr<Layer>>::iterator;
    using const_iterator = std::vector<std::shared_ptr<Layer>>::const_iterator;
    using reverse_iterator = std::vector<std::shared_ptr<Layer>>::reverse_iterator;
    using const_reverse_iterator = std::vector<std::shared_ptr<Layer>>::const_reverse_iterator;

    void add_layer(std::shared_ptr<Layer> layer);
    void remove_layer(std::shared_ptr<Layer> layer);
    void insert_layer(size_t index, std::shared_ptr<Layer> layer);

    bool move_layer(size_t from_index, size_t to_index);

    size_t count() const { return m_layers.size(); }
    bool empty() const { return m_layers.empty(); }

    std::shared_ptr<Layer> operator[](size_t index) const { return m_layers[index]; }

    iterator begin() { return m_layers.begin(); }
    iterator end() { return m_layers.end(); }
    const_iterator begin() const { return m_layers.begin(); }
    const_iterator end() const { return m_layers.end(); }

    reverse_iterator rbegin() { return m_layers.rbegin(); }
    reverse_iterator rend() { return m_layers.rend(); }
    const_reverse_iterator rbegin() const { return m_layers.rbegin(); }
    const_reverse_iterator rend() const { return m_layers.rend(); }

  private:
    std::vector<std::shared_ptr<Layer>> m_layers;
};

}  // namespace gimp
