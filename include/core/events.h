/**
 * @file events.h
 * @brief Core event types for the EventBus system.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace gimp {

class Layer;
class Document;

/**
 * @brief Event fired when the active layer selection changes.
 */
struct LayerSelectionChangedEvent {
    std::shared_ptr<Layer> previousLayer;  ///< The previously selected layer, or nullptr.
    std::shared_ptr<Layer> currentLayer;   ///< The newly selected layer, or nullptr.
    std::size_t layerIndex = 0;            ///< Index of the current layer in the stack.
};

/**
 * @brief Event fired when a layer property changes (name, visibility, opacity, blend mode).
 */
struct LayerPropertyChangedEvent {
    std::shared_ptr<Layer> layer;  ///< The layer whose property changed.
    std::string propertyName;      ///< Name of the changed property (e.g., "opacity", "visible").
};

/**
 * @brief Event fired when the layer stack changes (add, remove, reorder).
 */
struct LayerStackChangedEvent {
    /*! @brief Type of change that occurred. */
    enum class Action {
        Added,     ///< A layer was added.
        Removed,   ///< A layer was removed.
        Reordered  ///< Layers were reordered.
    };
    Action action = Action::Added;         ///< The action that occurred.
    std::shared_ptr<Layer> affectedLayer;  ///< The layer affected by the action.
};

/**
 * @brief Event fired when the undo/redo history changes.
 */
struct HistoryChangedEvent {
    std::size_t undoCount = 0;   ///< Number of available undo steps.
    std::size_t redoCount = 0;   ///< Number of available redo steps.
    std::string lastActionName;  ///< Name of the most recent action.
};

/**
 * @brief Event fired when the active tool changes.
 */
struct ToolChangedEvent {
    std::string previousToolId;  ///< ID of the previously active tool.
    std::string currentToolId;   ///< ID of the newly active tool.
};

/**
 * @brief Event fired when a tool property changes.
 */
struct ToolPropertyChangedEvent {
    std::string toolId;        ///< ID of the tool whose property changed.
    std::string propertyName;  ///< Name of the changed property.
};

/**
 * @brief Event fired when the active document changes.
 */
struct DocumentChangedEvent {
    std::shared_ptr<Document> document;  ///< The newly active document, or nullptr.
};

/**
 * @brief Event fired when the canvas view changes (pan, zoom).
 */
struct CanvasViewChangedEvent {
    float zoomLevel = 1.0F;  ///< Current zoom level (1.0 = 100%).
    float panX = 0.0F;       ///< Horizontal pan offset in pixels.
    float panY = 0.0F;       ///< Vertical pan offset in pixels.
};

/**
 * @brief Event fired when the mouse position changes on the canvas.
 */
struct MousePositionChangedEvent {
    int canvasX = 0;  ///< Mouse X position in canvas coordinates.
    int canvasY = 0;  ///< Mouse Y position in canvas coordinates.
    int screenX = 0;  ///< Mouse X position in screen coordinates.
    int screenY = 0;  ///< Mouse Y position in screen coordinates.
};

/**
 * @brief Event fired when the foreground or background color changes.
 */
struct ColorChangedEvent {
    std::uint32_t color = 0x000000FF;  ///< New color in RGBA format (0xRRGGBBAA).
    std::string source;                ///< Source of the change (e.g., "color_picker", "palette").
};

}  // namespace gimp
