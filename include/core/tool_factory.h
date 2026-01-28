/**
 * @file tool_factory.h
 * @brief Factory for creating and managing tool instances.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace gimp {

class Tool;
class Document;
class CommandBus;

/**
 * @brief Factory function type for creating tool instances.
 */
using ToolCreator = std::function<std::unique_ptr<Tool>()>;

/**
 * @brief Factory for creating and caching tool instances.
 *
 * Tools are created lazily and cached for reuse. The factory also handles
 * injecting dependencies (Document, CommandBus) into tools when they're
 * activated.
 */
class ToolFactory {
  public:
    /*! @brief Returns the singleton ToolFactory instance.
     *  @return Reference to the global ToolFactory.
     */
    static ToolFactory& instance();

    /**
     * @brief Registers a tool creator function.
     * @param toolId The unique tool identifier.
     * @param creator Function that creates the tool instance.
     */
    void registerTool(const std::string& toolId, ToolCreator creator);

    /**
     * @brief Gets or creates a tool instance by ID.
     * @param toolId The tool identifier.
     * @return Pointer to the tool, or nullptr if not registered.
     */
    [[nodiscard]] Tool* getTool(const std::string& toolId);

    /**
     * @brief Sets the active tool by ID.
     * @param toolId The tool to activate.
     * @return Pointer to the activated tool, or nullptr if not found.
     */
    Tool* setActiveTool(const std::string& toolId);

    /*! @brief Returns the currently active tool.
     *  @return Pointer to the active tool, or nullptr.
     */
    [[nodiscard]] Tool* activeTool() const { return activeTool_; }

    /*! @brief Returns the ID of the currently active tool.
     *  @return Active tool ID, or empty string.
     */
    [[nodiscard]] const std::string& activeToolId() const { return activeToolId_; }

    /*! @brief Returns the ID of the previously active tool.
     *  @return Previous tool ID, or empty string.
     */
    [[nodiscard]] const std::string& previousToolId() const { return previousToolId_; }

    /**
     * @brief Sets the document for all tools to operate on.
     * @param document The active document.
     */
    void setDocument(std::shared_ptr<Document> document);

    /**
     * @brief Sets the command bus for tools to issue commands.
     * @param commandBus The command bus instance.
     */
    void setCommandBus(CommandBus* commandBus);

    /**
     * @brief Clears all cached tool instances.
     */
    void clearCache();

  private:
    ToolFactory() = default;

    void injectDependencies(Tool* tool);

    std::unordered_map<std::string, ToolCreator> creators_;
    std::unordered_map<std::string, std::unique_ptr<Tool>> cache_;
    Tool* activeTool_ = nullptr;
    std::string activeToolId_;
    std::string previousToolId_;
    std::shared_ptr<Document> document_;
    CommandBus* commandBus_ = nullptr;
};

}  // namespace gimp
