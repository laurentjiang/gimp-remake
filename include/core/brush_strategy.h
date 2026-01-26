/**
 * @file brush_strategy.h
 * @brief Strategy pattern for brush rendering behaviors.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

/**
 * @brief Abstract strategy for brush dab rendering.
 *
 * Different brush strategies produce different visual effects:
 * - SolidBrush: Hard-edged single color
 * - StampBrush: Uses a texture/pattern image
 * - AirbrushStrategy: Soft edges with accumulating coverage
 */
class BrushStrategy {
  public:
    virtual ~BrushStrategy() = default;

    /**
     * @brief Renders a single brush dab at the given position.
     * @param target Pointer to the target pixel buffer (RGBA, 4 bytes per pixel).
     * @param targetWidth Width of the target buffer in pixels.
     * @param targetHeight Height of the target buffer in pixels.
     * @param x Center X position for the dab.
     * @param y Center Y position for the dab.
     * @param size Brush diameter in pixels.
     * @param color Base color in RGBA format (0xRRGGBBAA).
     * @param pressure Pen pressure (0.0 to 1.0), affects opacity/size.
     */
    virtual void renderDab(std::uint8_t* target, int targetWidth, int targetHeight, int x, int y,
                           int size, std::uint32_t color, float pressure) = 0;

    /*! @brief Returns a unique identifier for this strategy type.
     *  @return Strategy type name.
     */
    [[nodiscard]] virtual const char* typeName() const = 0;
};

/**
 * @brief Solid color brush with hard edges.
 *
 * Renders circular dabs with 100% hardness (no anti-aliasing or feathering).
 */
class SolidBrush : public BrushStrategy {
  public:
    void renderDab(std::uint8_t* target, int targetWidth, int targetHeight, int x, int y, int size,
                   std::uint32_t color, float pressure) override;

    [[nodiscard]] const char* typeName() const override { return "solid"; }
};

/**
 * @brief Stamp brush that uses a texture image.
 *
 * Each dab stamps a copy of the texture pattern, tinted by the current color.
 */
class StampBrush : public BrushStrategy {
  public:
    /**
     * @brief Sets the stamp texture.
     * @param data Grayscale alpha mask data (single channel).
     * @param width Texture width in pixels.
     * @param height Texture height in pixels.
     */
    void setStamp(std::vector<std::uint8_t> data, int width, int height);

    void renderDab(std::uint8_t* target, int targetWidth, int targetHeight, int x, int y, int size,
                   std::uint32_t color, float pressure) override;

    [[nodiscard]] const char* typeName() const override { return "stamp"; }

  private:
    std::vector<std::uint8_t> stampData_;
    int stampWidth_ = 0;
    int stampHeight_ = 0;
};

/**
 * @brief Creates a BrushStrategy from a type name.
 * @param typeName The strategy type ("solid", "stamp").
 * @return Unique pointer to the created strategy, or nullptr if unknown.
 */
std::unique_ptr<BrushStrategy> createBrushStrategy(const char* typeName);

}  // namespace gimp
