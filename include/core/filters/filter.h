/**
 * @file filter.h
 * @brief Abstract base class for image filters.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gimp {

class Layer;

/**
 * @brief Abstract base class for all image filters.
 *
 * Filters process layer pixel data and can be applied with configurable
 * parameters. They support progress reporting for long-running operations.
 */
class Filter {
  public:
    virtual ~Filter() = default;

    /**
     * @brief Returns the unique filter identifier.
     * @return Filter ID string (e.g., "blur", "sharpen").
     */
    [[nodiscard]] virtual std::string id() const = 0;

    /**
     * @brief Returns the human-readable filter name.
     * @return Filter display name.
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Returns a description of what the filter does.
     * @return Filter description.
     */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * @brief Applies the filter to a layer.
     * @param layer The layer to filter (modified in-place).
     * @return True if successful, false if an error occurred.
     * @pre layer must not be null.
     * @post Layer pixel data is modified according to filter parameters.
     */
    virtual bool apply(std::shared_ptr<Layer> layer) = 0;

    /**
     * @brief Sets a filter parameter by name.
     * @param name Parameter name.
     * @param value Parameter value (interpretation depends on parameter type).
     * @return True if parameter was recognized and set, false otherwise.
     */
    virtual bool setParameter(const std::string& name, float value) = 0;

    /**
     * @brief Gets a filter parameter value by name.
     * @param name Parameter name.
     * @param value Output parameter value.
     * @return True if parameter exists and was retrieved, false otherwise.
     */
    virtual bool getParameter(const std::string& name, float& value) const = 0;

    /**
     * @brief Returns the progress of the current operation (0.0 to 1.0).
     * @return Progress value.
     */
    [[nodiscard]] virtual float progress() const = 0;

    /**
     * @brief Checks if the filter is currently processing.
     * @return True if filter is running, false otherwise.
     */
    [[nodiscard]] virtual bool isRunning() const = 0;
};

}  // namespace gimp
