/**
 * @file sharpen_filter.h
 * @brief Sharpen filter implementation.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#pragma once

#include "filter.h"

namespace gimp {

/**
 * @brief Sharpen filter using unsharp masking.
 *
 * Applies sharpening by subtracting a blurred copy from the original,
 * which enhances edges. Configurable amount controls the strength.
 */
class SharpenFilter : public Filter {
  public:
    SharpenFilter() = default;

    [[nodiscard]] std::string id() const override { return "sharpen"; }
    [[nodiscard]] std::string name() const override { return "Sharpen"; }
    [[nodiscard]] std::string description() const override {
        return "Sharpen edges using unsharp masking";
    }

    /**
     * @brief Sets the sharpen amount.
     * @param amount Sharpening strength (0.0 to 2.0).
     *               0.0 = no effect, 1.0 = normal, 2.0 = strong.
     */
    void setAmount(float amount);

    /**
     * @brief Returns the current sharpen amount.
     * @return Sharpening strength.
     */
    [[nodiscard]] float amount() const { return amount_; }

    /**
     * @brief Sets the blur radius for unsharp masking.
     * @param radius Blur radius (1 to 50).
     */
    void setRadius(float radius);

    /**
     * @brief Returns the current blur radius.
     * @return Blur radius in pixels.
     */
    [[nodiscard]] float radius() const { return radius_; }

    bool apply(std::shared_ptr<Layer> layer) override;
    bool setParameter(const std::string& name, float value) override;
    bool getParameter(const std::string& name, float& value) const override;

  private:
    /**
     * @brief Creates a blurred copy of the layer for unsharp masking.
     * @param data Original layer data.
     * @param width Layer width.
     * @param height Layer height.
     * @return Blurred copy of the data.
     */
    std::vector<std::uint8_t> createBlurredCopy(const std::vector<std::uint8_t>& data,
                                                int width,
                                                int height);

    float amount_ = 1.0F;   ///< Sharpening strength (0.0-2.0).
    float radius_ = 1.0F;   ///< Blur radius for unsharp mask.
};

}  // namespace gimp