/**
 * @file layer_stack.cpp
 * @brief Implementation of LayerStack.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "layer_stack.h"
#include <algorithm>

namespace gimp {

void LayerStack::add_layer(std::shared_ptr<Layer> layer) {
    if (layer) {
        m_layers.push_back(layer);
    }
}

void LayerStack::remove_layer(std::shared_ptr<Layer> layer) {
    auto it = std::find(m_layers.begin(), m_layers.end(), layer);
    if (it != m_layers.end()) {
        m_layers.erase(it);
    }
}

void LayerStack::insert_layer(size_t index, std::shared_ptr<Layer> layer) {
    if (!layer) return;
    if (index >= m_layers.size()) {
        m_layers.push_back(layer);
    } else {
        m_layers.insert(m_layers.begin() + index, layer);
    }
}

bool LayerStack::move_layer(size_t from_index, size_t to_index) {
    if (from_index >= m_layers.size()) {
        return false;
    }
    
    // Clamp to_index to valid range [0, size-1]
    if (to_index >= m_layers.size()) {
        to_index = m_layers.size() - 1;
    }

    if (from_index == to_index) {
        return true;
    }

    auto layer = m_layers[from_index];
    m_layers.erase(m_layers.begin() + from_index);
    m_layers.insert(m_layers.begin() + to_index, layer);
    return true;
}

} // namespace gimp
