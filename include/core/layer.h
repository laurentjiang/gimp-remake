/**
 * @file layer.h
 * @brief Interface for compositable image layers.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gimp {

enum class BlendMode {
    Normal,
    Multiply,
    Overlay,
    Screen,
    Darken,
    Lighten
};

class Layer {
  public:
    virtual ~Layer() = default;

    Layer(int width, int height) : m_width(width), m_height(height)
    {
        m_data.resize(width * height * 4, 0);
    }

    void set_name(const std::string& name) { m_name = name; }
    const std::string& name() const { return m_name; }

    void set_visible(bool visible) { m_visible = visible; }
    bool visible() const { return m_visible; }

    void set_opacity(float opacity) { m_opacity = opacity; }
    float opacity() const { return m_opacity; }

    void set_blend_mode(BlendMode mode) { m_blend_mode = mode; }
    BlendMode blend_mode() const { return m_blend_mode; }

    int width() const { return m_width; }
    int height() const { return m_height; }

    std::vector<uint8_t>& data() { return m_data; }
    const std::vector<uint8_t>& data() const { return m_data; }

  private:
    std::string m_name = "Layer";
    bool m_visible = true;
    float m_opacity = 1.0f;
    BlendMode m_blend_mode = BlendMode::Normal;

    int m_width = 0;
    int m_height = 0;
    std::vector<uint8_t> m_data;
};

}  // namespace gimp
