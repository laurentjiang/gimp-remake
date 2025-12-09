/**
 * @file layer.h
 * @brief Interface for compositable image layers.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

namespace gimp {
class Layer {
  public:
    virtual ~Layer() = default;
};
}  // namespace gimp
