/**
 * @file utility.h
 * @brief Utility functions for IO operations.
 * @author Aless Tosi
 * @date 2026-01-11
 */

#pragma once

#include "core/layer.h"

#include <string>

namespace gimp {

    /**
     * @brief Convert BlendMode enum to string representation.
     * @param mode The blend mode to convert.
     * @return String representation of the blend mode.
     */
    inline std::string blend_mode_to_string(BlendMode mode) {
        switch (mode) {
            case BlendMode::Normal:
                return "Normal";
            case BlendMode::Multiply:
                return "Multiply";
            case BlendMode::Overlay:
                return "Overlay";
            case BlendMode::Screen:
                return "Screen";
            case BlendMode::Darken:
                return "Darken";
            case BlendMode::Lighten:
                return "Lighten";
            default:
                return "Normal";
        }
    }

    /**
     * @brief Convert string to BlendMode enum.
     * @param mode String representation of the blend mode.
     * @return The corresponding BlendMode enum value.
     */
    inline BlendMode string_to_blend_mode(const std::string& mode) {
        if (mode == "Multiply") return BlendMode::Multiply;
        if (mode == "Overlay") return BlendMode::Overlay;
        if (mode == "Screen") return BlendMode::Screen;
        if (mode == "Darken") return BlendMode::Darken;
        if (mode == "Lighten") return BlendMode::Lighten;
        return BlendMode::Normal;
    }

}  // namespace gimp
