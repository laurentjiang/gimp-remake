/**
 * @file brush_dynamics.h
 * @brief Brush dynamics system for simulating pressure from velocity, fade, and random.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <random>

namespace gimp {

/**
 * @brief Input sources for brush dynamics.
 *
 * Similar to GIMP's dynamics system, these sources can be combined
 * to affect brush properties like opacity, size, and hardness.
 */
struct DynamicsInput {
    float pressure = 1.0F;   ///< Tablet pressure (0-1), 1.0 for mouse.
    float velocity = 0.0F;   ///< Movement velocity (0-1), derived from speed.
    float direction = 0.0F;  ///< Movement direction (0-1, normalized angle).
    float fade = 1.0F;       ///< Fade over stroke length (1 at start, 0 at end).
    float random = 0.0F;     ///< Random value (0-1), regenerated per dab.
};

/**
 * @brief Configuration for which dynamics sources affect brush output.
 */
struct DynamicsConfig {
    bool usePressure = true;    ///< Use tablet pressure.
    bool useVelocity = false;   ///< Use mouse velocity (inverse: fast = light).
    bool useDirection = false;  ///< Use stroke direction.
    bool useFade = false;       ///< Fade stroke over distance.
    bool useRandom = false;     ///< Add randomness.

    float velocitySensitivity = 1.0F;  ///< Velocity effect strength (0-1).
    float fadeLength = 500.0F;         ///< Fade distance in pixels.
};

/**
 * @brief Brush dynamics processor.
 *
 * Calculates effective brush values based on input dynamics.
 * For mouse input without pressure, velocity and fade provide
 * natural-feeling strokes.
 */
class BrushDynamics {
  public:
    BrushDynamics() : rng_(std::random_device{}()), dist_(0.0F, 1.0F) {}

    /*! @brief Sets the dynamics configuration.
     *  @param config The dynamics settings to use.
     */
    void setConfig(const DynamicsConfig& config) { config_ = config; }

    /*! @brief Returns the current configuration (const).
     *  @return Const reference to the dynamics configuration.
     */
    [[nodiscard]] const DynamicsConfig& config() const { return config_; }

    /*! @brief Returns the current configuration for modification.
     *  @return Mutable reference to the dynamics configuration.
     */
    [[nodiscard]] DynamicsConfig& config() { return config_; }

    /*! @brief Starts a new stroke, resetting fade distance.
     */
    void beginStroke()
    {
        strokeDistance_ = 0.0F;
        lastX_ = 0;
        lastY_ = 0;
        hasLastPoint_ = false;
    }

    /*! @brief Updates dynamics for the current point.
     *  @param x Current X position.
     *  @param y Current Y position.
     *  @param tabletPressure Pressure from tablet (1.0 for mouse).
     *  @return Computed dynamics input values.
     */
    DynamicsInput update(int x, int y, float tabletPressure = 1.0F)
    {
        DynamicsInput input;
        input.pressure = tabletPressure;
        input.random = dist_(rng_);

        if (hasLastPoint_) {
            // Calculate velocity from distance
            float dx = static_cast<float>(x - lastX_);
            float dy = static_cast<float>(y - lastY_);
            float dist = std::sqrt(dx * dx + dy * dy);

            strokeDistance_ += dist;

            // Velocity: normalize to 0-1 range
            // Typical mouse movement is 1-50 pixels per event
            // Map so that ~20px = 0.5 velocity
            float rawVelocity = dist / 40.0F;
            input.velocity = std::clamp(rawVelocity, 0.0F, 1.0F);

            // Direction: atan2 normalized to 0-1
            if (dist > 0.5F) {
                float angle = std::atan2(dy, dx);
                input.direction = (angle + 3.14159265F) / (2.0F * 3.14159265F);
            }

            // Fade: decreases over stroke distance
            if (config_.fadeLength > 0) {
                input.fade = std::clamp(1.0F - (strokeDistance_ / config_.fadeLength), 0.0F, 1.0F);
            }
        }

        lastX_ = x;
        lastY_ = y;
        hasLastPoint_ = true;

        return input;
    }

    /*! @brief Computes effective pressure from dynamics inputs.
     *  @param input The dynamics input values.
     *  @return Combined pressure value (0-1).
     */
    [[nodiscard]] float computePressure(const DynamicsInput& input) const
    {
        float total = 0.0F;
        int factors = 0;

        if (config_.usePressure) {
            total += input.pressure;
            factors++;
        }

        if (config_.useVelocity) {
            // Inverse velocity: slow = high pressure, fast = low pressure
            float velocityFactor = 1.0F - input.velocity * config_.velocitySensitivity;
            total += velocityFactor;
            factors++;
        }

        if (config_.useFade) {
            total += input.fade;
            factors++;
        }

        if (config_.useRandom) {
            total += input.random;
            factors++;
        }

        if (factors == 0) {
            return 1.0F;
        }

        return std::clamp(total / static_cast<float>(factors), 0.0F, 1.0F);
    }

    /*! @brief Computes effective size multiplier from dynamics.
     *  @param input The dynamics input values.
     *  @return Size multiplier (0-1).
     */
    [[nodiscard]] float computeSizeMultiplier(const DynamicsInput& input) const
    {
        // Size typically uses pressure directly
        if (config_.usePressure) {
            return input.pressure;
        }
        if (config_.useVelocity) {
            return 1.0F - input.velocity * config_.velocitySensitivity * 0.5F;
        }
        return 1.0F;
    }

  private:
    DynamicsConfig config_;
    float strokeDistance_ = 0.0F;
    int lastX_ = 0;
    int lastY_ = 0;
    bool hasLastPoint_ = false;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;
};

}  // namespace gimp
