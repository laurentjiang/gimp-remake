/**
 * @file eraser_tool.h
 * @brief Eraser tool implementation.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#pragma once

#include "core/commands/draw_command.h"
#include "core/tool.h"
#include "core/tool_options.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace gimp {

/**
 * @brief An eraser tool that removes pixels by setting them to transparent.
 *
 * The eraser tool works similarly to the pencil but instead of painting color,
 * it sets the alpha channel to 0 (transparent). Collects stroke points during
 * the Active state and issues a command on commit.
 */
class EraserTool : public Tool, public ToolOptions {
  public:
    EraserTool() = default;

    [[nodiscard]] std::string id() const override { return "eraser"; }
    [[nodiscard]] std::string name() const override { return "Eraser"; }

    /*! @brief Sets the brush size in pixels.
     *  @param size Brush diameter (1 to 1000).
     */
    void setBrushSize(int size) override { brushSize_ = size; }

    /*! @brief Returns the current brush size.
     *  @return Brush diameter in pixels.
     */
    [[nodiscard]] int brushSize() const override { return brushSize_; }

    /*! @brief Sets the eraser hardness.
     *  @param hardness Value from 0.0 (soft edges) to 1.0 (hard edges).
     */
    void setHardness(float hardness) { hardness_ = std::clamp(hardness, 0.0F, 1.0F); }

    /*! @brief Returns the current eraser hardness.
     *  @return Hardness value (0.0 to 1.0).
     */
    [[nodiscard]] float hardness() const { return hardness_; }

    /*! @brief Sets the eraser opacity.
     *  @param opacity Value from 0.0 (transparent) to 1.0 (opaque).
     */
    void setOpacity(float opacity) { opacity_ = std::clamp(opacity, 0.0F, 1.0F); }

    /*! @brief Returns the current eraser opacity.
     *  @return Opacity value (0.0 to 1.0).
     */
    [[nodiscard]] float opacity() const { return opacity_; }

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
    void eraseAt(int x, int y, float pressure);

    std::vector<StrokePoint> strokePoints_;
    std::vector<uint8_t> beforeState_;    ///< Layer data before stroke for undo.
    std::shared_ptr<Layer> activeLayer_;  ///< Layer being erased during stroke.
    int brushSize_ = 10;
    float hardness_ = 0.5F;
    float opacity_ = 1.0F;
};

}  // namespace gimp
