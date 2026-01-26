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

/*!
 * @enum BlendMode
 * @brief Blending modes for compositing layers.
 */
enum class BlendMode {
    Normal,    ///< No blending, top layer replaces bottom.
    Multiply,  ///< Darkens by multiplying colors.
    Overlay,   ///< Combines Multiply and Screen.
    Screen,    ///< Lightens by inverting, multiplying, inverting.
    Darken,    ///< Keeps the darker pixel.
    Lighten    ///< Keeps the lighter pixel.
};

/*!
 * @class Layer
 * @brief A single compositable image layer with RGBA pixel data.
 */
class Layer {
  public:
    virtual ~Layer() = default;

    /*!
     * @brief Constructs a layer with the given dimensions.
     * @param width Layer width in pixels.
     * @param height Layer height in pixels.
     */
    Layer(int width, int height) : m_width(width), m_height(height)
    {
        m_data.resize(width * height * 4, 0);
    }

    /*! @brief Sets the layer name.
     *  @param name The new name for the layer.
     */
    void set_name(const std::string& name) { m_name = name; }

    /*! @brief Returns the layer name.
     *  @return The layer name.
     */
    const std::string& name() const { return m_name; }

    /*! @brief Sets layer visibility.
     *  @param visible True to show the layer, false to hide.
     */
    void set_visible(bool visible) { m_visible = visible; }

    /*! @brief Returns true if the layer is visible.
     *  @return Visibility state.
     */
    bool visible() const { return m_visible; }

    /*! @brief Sets layer opacity (0.0 to 1.0).
     *  @param opacity The new opacity value.
     */
    void set_opacity(float opacity) { m_opacity = opacity; }

    /*! @brief Returns the layer opacity.
     *  @return Opacity value (0.0 to 1.0).
     */
    float opacity() const { return m_opacity; }

    /*! @brief Sets the blend mode.
     *  @param mode The new blend mode.
     */
    void set_blend_mode(BlendMode mode) { m_blend_mode = mode; }

    /*! @brief Returns the blend mode.
     *  @return The current blend mode.
     */
    BlendMode blend_mode() const { return m_blend_mode; }

    /*! @brief Returns the layer width.
     *  @return Width in pixels.
     */
    int width() const { return m_width; }

    /*! @brief Returns the layer height.
     *  @return Height in pixels.
     */
    int height() const { return m_height; }

    /*! @brief Returns mutable access to pixel data (RGBA, 4 bytes per pixel).
     *  @return Reference to the pixel data vector.
     */
    std::vector<uint8_t>& data() { return m_data; }

    /*! @brief Returns const access to pixel data.
     *  @return Const reference to the pixel data vector.
     */
    const std::vector<uint8_t>& data() const { return m_data; }

  private:
    std::string m_name = "Layer";                ///< Layer display name.
    bool m_visible = true;                       ///< Visibility flag.
    float m_opacity = 1.0f;                      ///< Opacity (0.0 to 1.0).
    BlendMode m_blend_mode = BlendMode::Normal;  ///< Blend mode.

    int m_width = 0;              ///< Width in pixels.
    int m_height = 0;             ///< Height in pixels.
    std::vector<uint8_t> m_data;  ///< RGBA pixel buffer.
};

}  // namespace gimp
