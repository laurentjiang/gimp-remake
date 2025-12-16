/**
 * @file layer.h
 * @brief Interface for compositable image layers.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <string>

namespace gimp {

class Layer {
  public:
    virtual ~Layer() = default;

    void set_name(const std::string& name) { m_name = name; }
    const std::string& name() const { return m_name; }

    void set_visible(bool visible) { m_visible = visible; }
    bool visible() const { return m_visible; }

    void set_opacity(float opacity) { m_opacity = opacity; }
    float opacity() const { return m_opacity; }

  private:
    std::string m_name = "Layer";
    bool m_visible = true;
    float m_opacity = 1.0f;
};

}  // namespace gimp
