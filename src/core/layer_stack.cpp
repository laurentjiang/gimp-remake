/**
 * @file layer_stack.cpp
 * @brief Implementation of LayerStack.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "core/layer_stack.h"

#include <algorithm>

namespace gimp {

void LayerStack::addLayer(const std::shared_ptr<Layer>& layer)
{
    if (layer) {
        m_layers.push_back(layer);
    }
}

void LayerStack::removeLayer(const std::shared_ptr<Layer>& layer)
{
    auto it = std::find(m_layers.begin(), m_layers.end(), layer);
    if (it != m_layers.end()) {
        m_layers.erase(it);
    }
}

void LayerStack::insertLayer(size_t index, const std::shared_ptr<Layer>& layer)
{
    if (!layer)
        return;
    if (index >= m_layers.size()) {
        m_layers.push_back(layer);
    } else {
        m_layers.insert(m_layers.begin() + static_cast<std::ptrdiff_t>(index), layer);
    }
}

bool LayerStack::moveLayer(size_t fromIndex, size_t toIndex)
{
    if (fromIndex >= m_layers.size()) {
        return false;
    }

    // Clamp toIndex to valid range [0, size-1]
    if (toIndex >= m_layers.size()) {
        toIndex = m_layers.size() - 1;
    }

    if (fromIndex == toIndex) {
        return true;
    }

    auto layer = m_layers[fromIndex];
    m_layers.erase(m_layers.begin() + static_cast<std::ptrdiff_t>(fromIndex));
    m_layers.insert(m_layers.begin() + static_cast<std::ptrdiff_t>(toIndex), layer);
    return true;
}

}  // namespace gimp
