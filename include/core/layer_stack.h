/**
 * @file layer_stack.h
 * @brief Manages the stack of layers in a document.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include "layer.h"

#include <memory>
#include <vector>

namespace gimp {

/*!
 * @class LayerStack
 * @brief Ordered collection of layers with iteration and manipulation methods.
 */
class LayerStack {
  public:
    using iterator = std::vector<std::shared_ptr<Layer>>::iterator;
    using const_iterator = std::vector<std::shared_ptr<Layer>>::const_iterator;
    using reverse_iterator = std::vector<std::shared_ptr<Layer>>::reverse_iterator;
    using const_reverse_iterator = std::vector<std::shared_ptr<Layer>>::const_reverse_iterator;

    /*!
     * @brief Adds a layer to the top of the stack.
     * @param layer The layer to add.
     * @pre layer must not be nullptr.
     * @post count() is incremented by 1.
     */
    void add_layer(const std::shared_ptr<Layer>& layer);

    /*!
     * @brief Removes a layer from the stack.
     * @param layer The layer to remove.
     * @post If layer was in the stack, count() is decremented by 1.
     */
    void remove_layer(const std::shared_ptr<Layer>& layer);

    /*!
     * @brief Inserts a layer at a specific index.
     * @param index Insertion position.
     * @param layer The layer to insert.
     * @pre layer must not be nullptr.
     * @post count() is incremented by 1.
     */
    void insert_layer(size_t index, const std::shared_ptr<Layer>& layer);

    /*!
     * @brief Moves a layer from one index to another.
     * @param from_index Source position.
     * @param to_index Destination position.
     * @return True if the move succeeded.
     * @pre from_index must be less than count().
     */
    bool move_layer(size_t from_index, size_t to_index);

    /*! @brief Returns the number of layers. */
    size_t count() const { return m_layers.size(); }

    /*! @brief Returns true if the stack is empty. */
    bool empty() const { return m_layers.empty(); }

    /*! @brief Access layer by index. */
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
    std::vector<std::shared_ptr<Layer>> m_layers; ///< Layer storage.
};

}  // namespace gimp
