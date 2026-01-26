/**
 * @file tool_registry.h
 * @brief Registry for available tools in the application.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gimp {

/**
 * @brief Describes a tool available in the application.
 */
struct ToolDescriptor {
    std::string id;       ///< Unique tool identifier.
    std::string name;     ///< Human-readable tool name.
    std::string iconName; ///< Resource path to the tool icon.
    std::string shortcut; ///< Keyboard shortcut (e.g., "P" for paintbrush).
    std::string category; ///< Tool category (e.g., "Paint", "Selection").
};

/**
 * @brief Central registry for all available tools.
 */
class ToolRegistry {
  public:
    /*! @brief Returns the singleton ToolRegistry instance.
     *  @return Reference to the global ToolRegistry.
     */
    static ToolRegistry& instance()
    {
        static ToolRegistry registry;
        return registry;
    }

    /*! @brief Registers a new tool.
     *  @param descriptor The tool descriptor to register.
     */
    void registerTool(const ToolDescriptor& descriptor)
    {
        tools_[descriptor.id] = descriptor;
        orderedIds_.push_back(descriptor.id);
    }

    /*! @brief Retrieves a tool by ID.
     *  @param id The tool identifier.
     *  @return Pointer to the tool descriptor, or nullptr if not found.
     */
    [[nodiscard]] const ToolDescriptor* getTool(const std::string& id) const
    {
        auto it = tools_.find(id);
        return it != tools_.end() ? &it->second : nullptr;
    }

    /*! @brief Returns all registered tools in order.
     *  @return Vector of all tool descriptors.
     */
    [[nodiscard]] std::vector<ToolDescriptor> getAllTools() const
    {
        std::vector<ToolDescriptor> result;
        result.reserve(orderedIds_.size());
        for (const auto& id : orderedIds_) {
            auto it = tools_.find(id);
            if (it != tools_.end()) {
                result.push_back(it->second);
            }
        }
        return result;
    }

    /*! @brief Returns tools in a specific category.
     *  @param category The category to filter by.
     *  @return Vector of matching tool descriptors.
     */
    [[nodiscard]] std::vector<ToolDescriptor> getToolsByCategory(const std::string& category) const
    {
        std::vector<ToolDescriptor> result;
        for (const auto& id : orderedIds_) {
            auto it = tools_.find(id);
            if (it != tools_.end() && it->second.category == category) {
                result.push_back(it->second);
            }
        }
        return result;
    }

    /*! @brief Sets the active tool.
     *  @param id The tool ID to activate.
     */
    void setActiveTool(const std::string& id) { activeToolId_ = id; }
    /*! @brief Returns the active tool ID.
     *  @return The currently active tool ID.
     */
    [[nodiscard]] const std::string& getActiveTool() const { return activeToolId_; }

  private:
    ToolRegistry() { registerDefaultTools(); }

    void registerDefaultTools()
    {
        registerTool(
            {"select_rect", "Rectangle Select", ":/icons/select-rect.svg", "R", "Selection"});
        registerTool(
            {"select_ellipse", "Ellipse Select", ":/icons/select-ellipse.svg", "E", "Selection"});
        registerTool({"select_free", "Free Select", ":/icons/select-lasso.svg", "F", "Selection"});

        registerTool({"move", "Move", ":/icons/move.svg", "M", "Transform"});
        registerTool({"rotate", "Rotate", ":/icons/rotate.svg", "", "Transform"});
        registerTool({"scale", "Scale", ":/icons/scale.svg", "", "Transform"});
        registerTool({"crop", "Crop", ":/icons/crop.svg", "C", "Transform"});

        registerTool({"paintbrush", "Paintbrush", ":/icons/paintbrush.svg", "P", "Paint"});
        registerTool({"pencil", "Pencil", ":/icons/pencil.svg", "N", "Paint"});
        registerTool({"eraser", "Eraser", ":/icons/eraser.svg", "Shift+E", "Paint"});
        registerTool({"bucket_fill", "Bucket Fill", ":/icons/bucket-fill.svg", "Shift+B", "Paint"});
        registerTool({"gradient", "Gradient", ":/icons/gradient.svg", "G", "Paint"});

        registerTool({"text", "Text", ":/icons/text.svg", "T", "Other"});
        registerTool({"color_picker", "Color Picker", ":/icons/color-picker.svg", "O", "Other"});
        registerTool({"zoom", "Zoom", ":/icons/zoom.svg", "Z", "Other"});

        activeToolId_ = "paintbrush";
    }

    std::unordered_map<std::string, ToolDescriptor> tools_;
    std::vector<std::string> orderedIds_;
    std::string activeToolId_;
};

}  // namespace gimp
