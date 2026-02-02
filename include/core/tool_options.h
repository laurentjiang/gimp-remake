/**
 * @file tool_options.h
 * @brief Interface for tool-specific options/settings.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#pragma once

#include <string>
#include <variant>
#include <vector>

namespace gimp {

/**
 * @brief Represents a single tool option with metadata.
 */
struct ToolOption {
    enum class Type {
        Slider,
        Dropdown,
        Checkbox,
        ColorPicker
    };

    std::string id;                                     ///< Unique identifier for the option
    std::string label;                                  ///< Display name
    Type type;                                          ///< Option type
    std::variant<int, float, bool, std::string> value;  ///< Current value

    // For Slider type
    float minValue = 0.0F;
    float maxValue = 100.0F;
    float step = 1.0F;

    // For Dropdown type
    std::vector<std::string> choices;
    int selectedIndex = 0;
};

/**
 * @brief Interface for tools to expose their configurable options.
 */
class ToolOptions {
  public:
    virtual ~ToolOptions() = default;

    /**
     * @brief Get all available options for this tool.
     * @return Vector of ToolOption structures.
     */
    [[nodiscard]] virtual std::vector<ToolOption> getOptions() const = 0;

    /**
     * @brief Set an option value.
     * @param optionId The option identifier.
     * @param value The new value.
     */
    virtual void setOptionValue(const std::string& optionId,
                                const std::variant<int, float, bool, std::string>& value) = 0;

    /**
     * @brief Get a specific option value.
     * @param optionId The option identifier.
     * @return The current value.
     */
    [[nodiscard]] virtual std::variant<int, float, bool, std::string> getOptionValue(
        const std::string& optionId) const = 0;
};

}  // namespace gimp
