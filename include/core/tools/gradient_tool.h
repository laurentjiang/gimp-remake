/**
 * @file gradient_tool.h
 * @brief Gradient tool for drawing linear and radial gradients.
 * @author Aless Tosi
 * @date 2026-02-01
 */

#pragma once

#include "core/tool.h"
#include "core/tool_options.h"

#include <cstdint>
#include <memory>
#include <vector>

class QPoint;

namespace gimp {

class Document;
class Layer;
class DrawCommand;

/**
 * @brief Gradient mode enumeration.
 */
enum class GradientMode {
    Linear,  ///< Linear gradient from start to end point
    Radial   ///< Radial gradient from center point
};

/**
 * @brief Gradient fill mode.
 */
enum class GradientFill {
    ForegroundToBackground,  ///< Gradient from foreground to background color
    ForegroundToTransparent  ///< Gradient from foreground to transparent
};

/**
 * @brief A gradient tool that draws linear and radial gradients.
 *
 * The gradient tool supports:
 * - Linear gradients (drag from start to end point)
 * - Radial gradients (drag to define radius)
 * - Foreground-to-background and foreground-to-transparent modes
 * - Undo/redo via DrawCommand
 */
class GradientTool : public Tool, public ToolOptions {
  public:
    GradientTool() = default;

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string name() const override;

    // ToolOptions interface
    [[nodiscard]] std::vector<ToolOption> getOptions() const override;
    void setOptionValue(const std::string& optionId,
                        const std::variant<int, float, bool, std::string>& value) override;
    [[nodiscard]] std::variant<int, float, bool, std::string> getOptionValue(
        const std::string& optionId) const override;

    /**
     * @brief Sets the gradient mode (linear or radial).
     * @param mode The gradient mode to use.
     */
    void setMode(GradientMode mode) { mode_ = mode; }

    /**
     * @brief Returns the current gradient mode.
     * @return Current GradientMode.
     */
    [[nodiscard]] GradientMode mode() const { return mode_; }

    /**
     * @brief Sets the gradient fill mode.
     * @param fill The fill mode (foreground-to-background or foreground-to-transparent).
     */
    void setFill(GradientFill fill) { fill_ = fill; }

    /**
     * @brief Returns the current gradient fill mode.
     * @return Current GradientFill.
     */
    [[nodiscard]] GradientFill fill() const { return fill_; }

    /**
     * @brief Sets the gradient shape (for radial gradients).
     * @param startX Start X position.
     * @param startY Start Y position.
     * @param endX End X position.
     * @param endY End Y position.
     */
    void setGradientShape(int startX, int startY, int endX, int endY);

    /**
     * @brief Returns the gradient start point.
     * @return QPoint with start position.
     */
    [[nodiscard]] QPoint gradientStart() const;

    /**
     * @brief Returns the gradient end point.
     * @return QPoint with end position.
     */
    [[nodiscard]] QPoint gradientEnd() const;

    /**
     * @brief Interpolates between two colors.
     * @param color1 First color (RGBA).
     * @param color2 Second color (RGBA).
     * @param t Interpolation factor (0.0 to 1.0).
     * @return Interpolated color (RGBA).
     */
    static std::uint32_t lerpColor(std::uint32_t color1, std::uint32_t color2, float t);

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    /**
     * @brief Applies a linear gradient to the layer.
     * @param layer The target layer.
     * @param startColor Starting color (RGBA).
     * @param endColor Ending color (RGBA).
     */
    void applyLinearGradient(const std::shared_ptr<Layer>& layer,
                             std::uint32_t startColor,
                             std::uint32_t endColor) const;

    /**
     * @brief Applies a radial gradient to the layer.
     * @param layer The target layer.
     * @param startColor Starting color (RGBA).
     * @param endColor Ending color (RGBA).
     */
    void applyRadialGradient(const std::shared_ptr<Layer>& layer,
                             std::uint32_t startColor,
                             std::uint32_t endColor) const;

    GradientMode mode_ = GradientMode::Linear;
    GradientFill fill_ = GradientFill::ForegroundToBackground;

    int startX_ = 0;
    int startY_ = 0;
    int endX_ = 0;
    int endY_ = 0;

    std::vector<std::uint8_t> beforeState_;
    std::shared_ptr<DrawCommand> command_;
    std::shared_ptr<Layer> activeLayer_;  ///< Layer being drawn on during stroke.
};

}  // namespace gimp
