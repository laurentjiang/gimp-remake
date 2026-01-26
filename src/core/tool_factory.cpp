/**
 * @file tool_factory.cpp
 * @brief Implementation of ToolFactory.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tool_factory.h"

#include "core/tool.h"

namespace gimp {

ToolFactory& ToolFactory::instance()
{
    static ToolFactory factory;
    return factory;
}

void ToolFactory::registerTool(const std::string& toolId, ToolCreator creator)
{
    creators_[toolId] = std::move(creator);
}

Tool* ToolFactory::getTool(const std::string& toolId)
{
    auto cacheIt = cache_.find(toolId);
    if (cacheIt != cache_.end()) {
        return cacheIt->second.get();
    }

    auto creatorIt = creators_.find(toolId);
    if (creatorIt == creators_.end()) {
        return nullptr;
    }

    auto tool = creatorIt->second();
    if (!tool) {
        return nullptr;
    }

    injectDependencies(tool.get());
    Tool* rawPtr = tool.get();
    cache_[toolId] = std::move(tool);
    return rawPtr;
}

Tool* ToolFactory::setActiveTool(const std::string& toolId)
{
    if (activeTool_) {
        activeTool_->onDeactivate();
    }

    Tool* newTool = getTool(toolId);
    if (newTool) {
        activeTool_ = newTool;
        activeToolId_ = toolId;
        newTool->onActivate();
    }
    return newTool;
}

void ToolFactory::setDocument(std::shared_ptr<Document> document)
{
    document_ = std::move(document);
    for (auto& [id, tool] : cache_) {
        tool->setDocument(document_);
    }
}

void ToolFactory::setCommandBus(CommandBus* commandBus)
{
    commandBus_ = commandBus;
    for (auto& [id, tool] : cache_) {
        tool->setCommandBus(commandBus_);
    }
}

void ToolFactory::clearCache()
{
    if (activeTool_) {
        activeTool_->onDeactivate();
    }
    activeTool_ = nullptr;
    activeToolId_.clear();
    cache_.clear();
}

void ToolFactory::injectDependencies(Tool* tool)
{
    if (document_) {
        tool->setDocument(document_);
    }
    if (commandBus_) {
        tool->setCommandBus(commandBus_);
    }
}

}  // namespace gimp
