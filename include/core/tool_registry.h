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
    std::string id;
    std::string name;
    std::string iconName;
    std::string shortcut;
    std::string category;
};

/**
 * @brief Central registry for all available tools.
 */
class ToolRegistry {
  public:
    static ToolRegistry& instance()
    {
        static ToolRegistry registry;
        return registry;
    }

    void registerTool(const ToolDescriptor& descriptor)
    {
        tools_[descriptor.id] = descriptor;
        orderedIds_.push_back(descriptor.id);
    }

    [[nodiscard]] const ToolDescriptor* getTool(const std::string& id) const
    {
        auto it = tools_.find(id);
        return it != tools_.end() ? &it->second : nullptr;
    }

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

    void setActiveTool(const std::string& id) { activeToolId_ = id; }
    [[nodiscard]] const std::string& getActiveTool() const { return activeToolId_; }

  private:
    ToolRegistry()
    {
        registerDefaultTools();
    }

    void registerDefaultTools()
    {
        registerTool({"select_rect", "Rectangle Select", "select-rectangular", "R", "Selection"});
        registerTool({"select_ellipse", "Ellipse Select", "select-elliptical", "E", "Selection"});
        registerTool({"select_free", "Free Select", "select-lasso", "F", "Selection"});

        registerTool({"move", "Move", "move", "M", "Transform"});
        registerTool({"rotate", "Rotate", "rotate", "", "Transform"});
        registerTool({"scale", "Scale", "scale", "", "Transform"});
        registerTool({"crop", "Crop", "crop", "C", "Transform"});

        registerTool({"paintbrush", "Paintbrush", "paintbrush", "P", "Paint"});
        registerTool({"pencil", "Pencil", "pencil", "N", "Paint"});
        registerTool({"eraser", "Eraser", "eraser", "Shift+E", "Paint"});
        registerTool({"bucket_fill", "Bucket Fill", "bucket-fill", "Shift+B", "Paint"});
        registerTool({"gradient", "Gradient", "gradient", "G", "Paint"});

        registerTool({"text", "Text", "text", "T", "Other"});
        registerTool({"color_picker", "Color Picker", "color-picker", "O", "Other"});
        registerTool({"zoom", "Zoom", "zoom", "Z", "Other"});

        activeToolId_ = "paintbrush";
    }

    std::unordered_map<std::string, ToolDescriptor> tools_;
    std::vector<std::string> orderedIds_;
    std::string activeToolId_;
};

}  // namespace gimp
