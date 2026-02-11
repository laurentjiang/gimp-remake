/**
 * @file brush_tool.h
 * @brief Brush tool with configurable hardness and opacity.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#pragma once

#include "core/brush_dynamics.h"
#include "core/brush_strategy.h"
#include "core/commands/draw_command.h"
#include "core/tool.h"
#include "core/tool_factory.h"
#include "core/tool_options.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

/**
 * @brief A brush tool with configurable hardness and opacity.
 *
 * The brush tool uses a SoftBrush strategy to render strokes with
 * variable edge softness controlled by the hardness parameter.
 * Opacity controls the overall transparency of the stroke.
 */
class BrushTool : public Tool, public ToolOptions {
  public:
    BrushTool();

    [[nodiscard]] std::string id() const override { return "paintbrush"; }
    [[nodiscard]] std::string name() const override { return "Paintbrush"; }

    /*! @brief Sets the brush size in pixels.
     *  @param size Brush diameter (1 to 1000).
     */
    void setBrushSize(int size) override { brushSize_ = size; }

    /*! @brief Returns the current brush size.
     *  @return Brush diameter in pixels.
     */
    [[nodiscard]] int brushSize() const override { return brushSize_; }

    /*! @brief Sets the brush hardness.
     *  @param hardness Value from 0.0 (soft edges) to 1.0 (hard edges).
     */
    void setHardness(float hardness);

    /*! @brief Returns the current brush hardness.
     *  @return Hardness value (0.0 to 1.0).
     */
    [[nodiscard]] float hardness() const { return hardness_; }

    /*! @brief Sets the brush opacity.
     *  @param opacity Value from 0.0 (transparent) to 1.0 (opaque).
     */
    void setOpacity(float opacity);

    /*! @brief Returns the current brush opacity.
     *  @return Opacity value (0.0 to 1.0).
     */
    [[nodiscard]] float opacity() const { return opacity_; }

    /*! @brief Sets the drawing color (updates global foreground color).
     *  @param rgba Color in RGBA format (0xRRGGBBAA).
     */
    void setColor(std::uint32_t rgba) { ToolFactory::instance().setForegroundColor(rgba); }

    /*! @brief Returns the current drawing color.
     *  @return Color in RGBA format (0xRRGGBBAA).
     */
    [[nodiscard]] std::uint32_t color() const { return ToolFactory::instance().foregroundColor(); }

    /*! @brief Enables or disables velocity-based dynamics.
     *  @param enabled True to enable velocity dynamics.
     */
    void setVelocityDynamics(bool enabled);

    /*! @brief Returns whether velocity dynamics is enabled.
     *  @return True if velocity dynamics is enabled.
     */
    [[nodiscard]] bool velocityDynamics() const { return dynamics_.config().useVelocity; }

    /*! @brief Returns a reference to the dynamics configuration.
     *  @return Reference to DynamicsConfig for full customization.
     */
    [[nodiscard]] DynamicsConfig& dynamicsConfig() { return dynamics_.config(); }

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

    std::unique_ptr<SoftBrush> brush_;
    BrushDynamics dynamics_;
    std::vector<StrokePoint> strokePoints_;
    std::vector<uint8_t> beforeState_;    ///< Layer data before stroke for undo.
    std::shared_ptr<Layer> activeLayer_;  ///< Layer being drawn on during stroke.
    int brushSize_ = 20;
    float hardness_ = 0.5F;
    float opacity_ = 1.0F;
};

}  // namespace gimp
