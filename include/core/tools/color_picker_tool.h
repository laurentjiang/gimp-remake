/**
 * @file color_picker_tool.h
 * @brief Color picker (eyedropper) tool implementation.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#pragma once

#include "core/tool.h"

#include <cstdint>
#include <string>

namespace gimp {

/**
 * @brief Color picker tool that samples pixel color from the canvas.
 *
 * The color picker tool samples the color at the clicked position from the
 * active layer and emits a ColorChangedEvent to update the foreground color.
 * After sampling, it automatically switches back to the previously active tool.
 */
class ColorPickerTool : public Tool {
  public:
    ColorPickerTool() = default;

    [[nodiscard]] std::string id() const override { return "color_picker"; }
    [[nodiscard]] std::string name() const override { return "Color Picker"; }

    /*! @brief Returns the last picked color.
     *  @return Color in RGBA format (0xRRGGBBAA).
     */
    [[nodiscard]] std::uint32_t pickedColor() const { return pickedColor_; }

    /*! @brief Sets the tool to switch back to after picking.
     *  @param toolId The previous tool ID.
     */
    void setPreviousTool(const std::string& toolId) { previousToolId_ = toolId; }

    void onActivate() override;

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override {}

  private:
    /**
     * @brief Samples the color at the given canvas position.
     * @param x X coordinate in canvas space.
     * @param y Y coordinate in canvas space.
     * @return Sampled color in RGBA format, or transparent black if out of bounds.
     */
    std::uint32_t sampleColorAt(int x, int y) const;

    /**
     * @brief Publishes a ColorChangedEvent with the given color.
     * @param color The new foreground color in RGBA format.
     */
    void publishColorChanged(std::uint32_t color) const;

    void requestSwitchToPreviousTool() const;

    std::uint32_t pickedColor_ = 0x000000FF;  ///< Last picked color.
    std::string previousToolId_;              ///< Tool to switch back to after picking.
};

}  // namespace gimp
