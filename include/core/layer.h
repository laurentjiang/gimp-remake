/**
 * @file layer.h
 * @brief Interface for compositable image layers.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
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
    void setName(const std::string& name) { m_name = name; }

    /*! @brief Returns the layer name.
     *  @return The layer name.
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /*! @brief Sets layer visibility.
     *  @param visible True to show the layer, false to hide.
     */
    void setVisible(bool visible) { m_visible = visible; }

    /*! @brief Returns true if the layer is visible.
     *  @return Visibility state.
     */
    [[nodiscard]] bool visible() const { return m_visible; }

    /*! @brief Sets layer opacity (0.0 to 1.0).
     *  @param opacity The new opacity value.
     */
    void setOpacity(float opacity) { m_opacity = opacity; }

    /*! @brief Returns the layer opacity.
     *  @return Opacity value (0.0 to 1.0).
     */
    [[nodiscard]] float opacity() const { return m_opacity; }

    /*! @brief Sets the blend mode.
     *  @param mode The new blend mode.
     */
    void setBlendMode(BlendMode mode) { m_blend_mode = mode; }

    /*! @brief Returns the blend mode.
     *  @return The current blend mode.
     */
    [[nodiscard]] BlendMode blendMode() const { return m_blend_mode; }

    /*! @brief Returns the layer width.
     *  @return Width in pixels.
     */
    [[nodiscard]] int width() const { return m_width; }

    /*! @brief Returns the layer height.
     *  @return Height in pixels.
     */
    [[nodiscard]] int height() const { return m_height; }

    /*! @brief Returns mutable access to pixel data (RGBA, 4 bytes per pixel).
     *  @return Reference to the pixel data vector.
     */
    std::vector<uint8_t>& data() { return m_data; }

    /*! @brief Returns const access to pixel data.
     *  @return Const reference to the pixel data vector.
     */
    [[nodiscard]] const std::vector<uint8_t>& data() const { return m_data; }

    /*! @brief Resizes the layer and repositions existing content.
     *  @param width New width in pixels.
     *  @param height New height in pixels.
     *  @param offsetX Horizontal offset applied to existing pixels.
     *  @param offsetY Vertical offset applied to existing pixels.
     */
    void resize(int width, int height, int offsetX, int offsetY)
    {
        if (width <= 0 || height <= 0) {
            m_width = std::max(0, width);
            m_height = std::max(0, height);
            m_data.clear();
            return;
        }

        std::vector<uint8_t> newData;
        newData.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U, 0);

        const int srcX = std::max(0, -offsetX);
        const int srcY = std::max(0, -offsetY);
        const int dstX = std::max(0, offsetX);
        const int dstY = std::max(0, offsetY);

        const int copyWidth = std::min(m_width - srcX, width - dstX);
        const int copyHeight = std::min(m_height - srcY, height - dstY);

        if (copyWidth > 0 && copyHeight > 0) {
            const int srcStride = m_width * 4;
            const int dstStride = width * 4;
            const size_t rowBytes = static_cast<size_t>(copyWidth) * 4U;

            for (int row = 0; row < copyHeight; ++row) {
                const size_t srcOffset =
                    static_cast<size_t>(srcY + row) * static_cast<size_t>(srcStride) +
                    static_cast<size_t>(srcX) * 4U;
                const size_t dstOffset =
                    static_cast<size_t>(dstY + row) * static_cast<size_t>(dstStride) +
                    static_cast<size_t>(dstX) * 4U;

                std::memcpy(newData.data() + dstOffset, m_data.data() + srcOffset, rowBytes);
            }
        }

        m_width = width;
        m_height = height;
        m_data = std::move(newData);
    }

  private:
    std::string m_name = "Layer";                ///< Layer display name.
    bool m_visible = true;                       ///< Visibility flag.
    float m_opacity = 1.0F;                      ///< Opacity (0.0 to 1.0).
    BlendMode m_blend_mode = BlendMode::Normal;  ///< Blend mode.

    int m_width = 0;              ///< Width in pixels.
    int m_height = 0;             ///< Height in pixels.
    std::vector<uint8_t> m_data;  ///< RGBA pixel buffer.
};

}  // namespace gimp
