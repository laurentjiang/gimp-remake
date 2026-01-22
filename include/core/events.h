/**
 * @file events.h
 * @brief Core event types for the EventBus system.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace gimp {

class Layer;
class Document;

/**
 * @brief Event fired when the active layer selection changes.
 */
struct LayerSelectionChangedEvent {
    std::shared_ptr<Layer> previousLayer;
    std::shared_ptr<Layer> currentLayer;
    std::size_t layerIndex = 0;
};

/**
 * @brief Event fired when a layer property changes (name, visibility, opacity, blend mode).
 */
struct LayerPropertyChangedEvent {
    std::shared_ptr<Layer> layer;
    std::string propertyName;
};

/**
 * @brief Event fired when the layer stack changes (add, remove, reorder).
 */
struct LayerStackChangedEvent {
    enum class Action { Added, Removed, Reordered };
    Action action = Action::Added;
    std::shared_ptr<Layer> affectedLayer;
};

/**
 * @brief Event fired when the undo/redo history changes.
 */
struct HistoryChangedEvent {
    std::size_t undoCount = 0;
    std::size_t redoCount = 0;
    std::string lastActionName;
};

/**
 * @brief Event fired when the active tool changes.
 */
struct ToolChangedEvent {
    std::string previousToolId;
    std::string currentToolId;
};

/**
 * @brief Event fired when a tool property changes.
 */
struct ToolPropertyChangedEvent {
    std::string toolId;
    std::string propertyName;
};

/**
 * @brief Event fired when the active document changes.
 */
struct DocumentChangedEvent {
    std::shared_ptr<Document> document;
};

/**
 * @brief Event fired when the canvas view changes (pan, zoom).
 */
struct CanvasViewChangedEvent {
    float zoomLevel = 1.0F;
    float panX = 0.0F;
    float panY = 0.0F;
};

/**
 * @brief Event fired when the mouse position changes on the canvas.
 */
struct MousePositionChangedEvent {
    int canvasX = 0;
    int canvasY = 0;
    int screenX = 0;
    int screenY = 0;
};

}  // namespace gimp
