/**
 * @file pencil_tool.h
 * @brief Pencil drawing tool implementation.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include "core/commands/draw_command.h"
#include "core/tool.h"
#include "core/tool_factory.h"
#include "core/tool_options.h"

#include <cstdint>
#include <vector>

namespace gimp {

/**
 * @brief A basic pencil tool that draws hard-edged lines.
 *
 * The pencil tool draws with a solid color at 100% hardness (no anti-aliasing).
 * It collects stroke points during the Active state and issues a command
 * on commit.
 */
class PencilTool : public Tool, public ToolOptions {
  public:
    PencilTool() = default;

    [[nodiscard]] std::string id() const override { return "pencil"; }
    [[nodiscard]] std::string name() const override { return "Pencil"; }

    /*! @brief Sets the brush size in pixels.
     *  @param size Brush diameter (1 to 1000).
     */
    void setBrushSize(int size) override { brushSize_ = size; }

    /*! @brief Returns the current brush size.
     *  @return Brush diameter in pixels.
     */
    [[nodiscard]] int brushSize() const override { return brushSize_; }

    /*! @brief Sets the drawing color (updates global foreground color).
     *  @param rgba Color in RGBA format (0xRRGGBBAA).
     */
    void setColor(std::uint32_t rgba) { ToolFactory::instance().setForegroundColor(rgba); }

    /*! @brief Returns the current drawing color (global foreground color).
     *  @return Color in RGBA format.
     */
    [[nodiscard]] std::uint32_t color() const { return ToolFactory::instance().foregroundColor(); }

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

    // ToolOptions interface
    [[nodiscard]] std::vector<ToolOption> getOptions() const override;
    void setOptionValue(const std::string& optionId,
                        const std::variant<int, float, bool, std::string>& value) override;
    [[nodiscard]] std::variant<int, float, bool, std::string> getOptionValue(
        const std::string& optionId) const override;

  private:
    /**
     * @brief Single point in a stroke.
     */
    struct StrokePoint {
        int x = 0;              ///< X coordinate in canvas space.
        int y = 0;              ///< Y coordinate in canvas space.
        float pressure = 1.0F;  ///< Pen pressure.
    };

    std::shared_ptr<DrawCommand> buildDrawCommand(int minX, int maxX, int minY, int maxY);
    void renderSegment(int fromX,
                       int fromY,
                       float fromPressure,
                       int toX,
                       int toY,
                       float toPressure);

    std::vector<StrokePoint> strokePoints_;
    std::vector<uint8_t> beforeState_;  ///< Layer data before stroke for undo.
    std::shared_ptr<Layer> activeLayer_;  ///< Layer being drawn on during stroke.
    int brushSize_ = 3;
    float opacity_ = 1.0F;  ///< Opacity/alpha value (0.0 to 1.0)
};

}  // namespace gimp
