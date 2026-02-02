/**
 * @file blur_filter.h
 * @brief Gaussian blur filter implementation.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#pragma once

#include "filter.h"

namespace gimp {

/**
 * @brief Gaussian blur filter.
 *
 * Applies a Gaussian blur with configurable radius.
 * Larger radius values produce stronger blur effects.
 */
class BlurFilter : public Filter {
  public:
    BlurFilter() = default;

    [[nodiscard]] std::string id() const override { return "blur"; }
    [[nodiscard]] std::string name() const override { return "Blur"; }
    [[nodiscard]] std::string description() const override {
        return "Apply Gaussian blur to the layer";
    }

    /**
     * @brief Sets the blur radius in pixels.
     * @param radius Blur radius (1 to 100). Higher values = stronger blur.
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
    float progress() const override;
    bool isRunning() const override;

  private:
    /**
     * @brief Applies horizontal blur pass.
     * @param data Layer pixel data (RGBA).
     * @param width Layer width.
     * @param height Layer height.
     * @param kernel Precomputed blur kernel.
     */
    void applyHorizontalBlur(std::vector<std::uint8_t>& data,
                             int width,
                             int height,
                             const std::vector<float>& kernel);

    /**
     * @brief Applies vertical blur pass.
     * @param data Layer pixel data (RGBA).
     * @param width Layer width.
     * @param height Layer height.
     * @param kernel Precomputed blur kernel.
     */
    void applyVerticalBlur(std::vector<std::uint8_t>& data,
                           int width,
                           int height,
                           const std::vector<float>& kernel);

    /**
     * @brief Generates a Gaussian blur kernel.
     * @param radius Blur radius.
     * @return Vector of kernel coefficients.
     */
    static std::vector<float> generateGaussianKernel(float radius);

    float radius_ = 5.0F;  ///< Blur radius in pixels.
};

}  // namespace gimp