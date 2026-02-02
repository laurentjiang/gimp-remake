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
    std::string id;         ///< Unique tool identifier.
    std::string name;       ///< Human-readable tool name.
    std::string iconName;   ///< Resource path to the tool icon.
    std::string shortcut;   ///< Keyboard shortcut (e.g., "P" for paintbrush).
    std::string category;   ///< Tool category (e.g., "Paint", "Selection").
    std::string groupId;    ///< Tool group ID for grouping similar tools (empty if standalone).
    bool isPrimary = true;  ///< Whether this is the primary tool shown in the toolbox.
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

    /*! @brief Returns tools in a specific group.
     *  @param groupId The group ID to filter by.
     *  @return Vector of matching tool descriptors.
     */
    [[nodiscard]] std::vector<ToolDescriptor> getToolsByGroup(const std::string& groupId) const
    {
        std::vector<ToolDescriptor> result;
        for (const auto& id : orderedIds_) {
            auto it = tools_.find(id);
            if (it != tools_.end() && it->second.groupId == groupId) {
                result.push_back(it->second);
            }
        }
        return result;
    }

    /*! @brief Returns only primary tools (shown in toolbox).
     *  @return Vector of primary tool descriptors.
     */
    [[nodiscard]] std::vector<ToolDescriptor> getPrimaryTools() const
    {
        std::vector<ToolDescriptor> result;
        for (const auto& id : orderedIds_) {
            auto it = tools_.find(id);
            if (it != tools_.end() && it->second.isPrimary) {
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
        // Selection tools - grouped
        registerTool({"select_rect",
                      "Rectangle Select",
                      ":/icons/select-rect.svg",
                      "R",
                      "Selection",
                      "selection",
                      true});
        registerTool({"select_ellipse",
                      "Ellipse Select",
                      ":/icons/select-ellipse.svg",
                      "E",
                      "Selection",
                      "selection",
                      false});
        registerTool({"select_free",
                      "Free Select",
                      ":/icons/select-lasso.svg",
                      "F",
                      "Selection",
                      "selection",
                      false});

        // Transform tools - standalone
        registerTool({"move", "Move", ":/icons/move.svg", "M", "Transform", "", true});
        registerTool(
            {"rotate", "Rotate", ":/icons/rotate.svg", "", "Transform", "transform", true});
        registerTool({"scale", "Scale", ":/icons/scale.svg", "", "Transform", "transform", false});
        registerTool({"crop", "Crop", ":/icons/crop.svg", "C", "Transform", "", true});

        // Paint tools - paintbrush group
        registerTool(
            {"paintbrush", "Paintbrush", ":/icons/paintbrush.svg", "P", "Paint", "brush", true});
        registerTool({"pencil", "Pencil", ":/icons/pencil.svg", "N", "Paint", "brush", false});
        registerTool({"eraser", "Eraser", ":/icons/eraser.svg", "Shift+E", "Paint", "", true});
        registerTool({"bucket_fill",
                      "Bucket Fill",
                      ":/icons/bucket-fill.svg",
                      "Shift+B",
                      "Paint",
                      "",
                      true});
        registerTool({"gradient", "Gradient", ":/icons/gradient.svg", "G", "Paint", "", true});

        // Other tools - standalone
        registerTool({"text", "Text", ":/icons/text.svg", "T", "Other", "", true});
        registerTool(
            {"color_picker", "Color Picker", ":/icons/color-picker.svg", "O", "Other", "", true});
        registerTool({"zoom", "Zoom", ":/icons/zoom.svg", "Z", "Other", "", true});

        activeToolId_ = "paintbrush";
    }

    std::unordered_map<std::string, ToolDescriptor> tools_;
    std::vector<std::string> orderedIds_;
    std::string activeToolId_;
};

}  // namespace gimp
