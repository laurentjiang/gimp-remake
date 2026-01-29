/**
 * @file fill_tool.h
 * @brief Fill (bucket) tool implementation.
 * @author Laurent Jiang
 * @date 2026-01-29
 */

#pragma once

#include "core/tool.h"
#include "core/tool_factory.h"

#include <cstdint>
#include <vector>

namespace gimp {

/**
 * @brief A flood-fill tool that fills contiguous regions with a color.
 *
 * The fill tool uses a scanline flood-fill algorithm for efficient filling.
 * It supports tolerance settings to control color matching threshold.
 */
class FillTool : public Tool {
  public:
    FillTool() = default;

    [[nodiscard]] std::string id() const override { return "bucket_fill"; }
    [[nodiscard]] std::string name() const override { return "Bucket Fill"; }

    /*! @brief Sets the color tolerance for matching (0-255).
     *  @param tolerance Maximum color difference per channel to consider a match.
     */
    void setTolerance(int tolerance);

    /*! @brief Returns the current color tolerance.
     *  @return Tolerance value (0-255).
     */
    [[nodiscard]] int tolerance() const { return tolerance_; }

    /*! @brief Sets the fill color (updates global foreground color).
     *  @param rgba Color in RGBA format (0xRRGGBBAA).
     */
    void setColor(std::uint32_t rgba) { ToolFactory::instance().setForegroundColor(rgba); }

    /*! @brief Returns the current fill color (global foreground color).
     *  @return Color in RGBA format.
     */
    [[nodiscard]] std::uint32_t color() const { return ToolFactory::instance().foregroundColor(); }

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    /**
     * @brief Performs flood fill starting from the given coordinates.
     * @param startX Starting X coordinate.
     * @param startY Starting Y coordinate.
     * @param fillColor The color to fill with (RGBA).
     */
    void floodFill(int startX, int startY, std::uint32_t fillColor);

    /**
     * @brief Checks if a pixel's color matches the target within tolerance.
     * @param pixelColor The pixel color to check (RGBA).
     * @param targetColor The target color to match against (RGBA).
     * @return True if colors match within tolerance.
     */
    [[nodiscard]] bool colorMatches(std::uint32_t pixelColor, std::uint32_t targetColor) const;

    /**
     * @brief Gets the color at a specific pixel position.
     * @param data Pixel data buffer.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param width Layer width.
     * @return The color at the position (RGBA).
     */
    static std::uint32_t getPixelColor(const std::vector<uint8_t>& data, int x, int y, int width);

    /**
     * @brief Sets the color at a specific pixel position.
     * @param data Pixel data buffer.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param width Layer width.
     * @param color The color to set (RGBA).
     */
    static void setPixelColor(std::vector<uint8_t>& data,
                              int x,
                              int y,
                              int width,
                              std::uint32_t color);

    std::vector<uint8_t> beforeState_;  ///< Layer data before fill for undo.
    int tolerance_ = 0;                 ///< Color matching tolerance (0-255).
    bool fillPending_ = false;          ///< Whether a fill operation is pending commit.
};

}  // namespace gimp
