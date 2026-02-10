/**
 * @file skia_canvas_widget.h
 * @brief Interactive canvas widget with pan, zoom, tool input handling, and performance
 * measurement.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include <QImage>
#include <QPixmap>
#include <QPointF>
#include <QTimer>
#include <QWidget>

#include <memory>

namespace gimp {

class Document;
class SkiaRenderer;
class Tool;

/**
 * @brief Viewport transformation state for pan and zoom.
 */
struct ViewportState {
    float zoomLevel = 1.0F;  ///< Current zoom level (1.0 = 100%).
    float panX = 0.0F;       ///< Horizontal pan offset in widget pixels.
    float panY = 0.0F;       ///< Vertical pan offset in widget pixels.

    static constexpr float MIN_ZOOM = 0.1F;    ///< Minimum zoom level (10%).
    static constexpr float MAX_ZOOM = 32.0F;   ///< Maximum zoom level (3200%).
    static constexpr float ZOOM_STEP = 1.25F;  ///< Zoom factor per scroll step.
};

/**
 * @brief Interactive canvas widget that displays a document rendered via Skia.
 *
 * Provides:
 * - Pan (middle mouse drag or Space+drag)
 * - Zoom (Ctrl+scroll wheel)
 * - Scroll (plain scroll wheel)
 * - Tool dispatch (forwards mouse events to the active tool)
 * - Coordinate transformation between screen and canvas space
 * - Cursor management based on active tool and state
 */
class SkiaCanvasWidget : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructs the canvas widget.
     * @param document The document to display.
     * @param renderer The Skia renderer for compositing.
     * @param parent Optional parent widget.
     */
    explicit SkiaCanvasWidget(std::shared_ptr<Document> document,
                              std::shared_ptr<SkiaRenderer> renderer,
                              QWidget* parent = nullptr);
    ~SkiaCanvasWidget() override;

    /**
     * @brief Converts screen coordinates to canvas coordinates.
     * @param screenPos Position in widget coordinates.
     * @return Position in canvas (document) coordinates.
     */
    [[nodiscard]] QPointF screenToCanvas(const QPoint& screenPos) const;

    /**
     * @brief Converts canvas coordinates to screen coordinates.
     * @param canvasPos Position in canvas (document) coordinates.
     * @return Position in widget coordinates.
     */
    [[nodiscard]] QPoint canvasToScreen(const QPointF& canvasPos) const;

    /*! @brief Returns the current viewport state.
     *  @return Reference to the viewport state.
     */
    [[nodiscard]] const ViewportState& viewport() const { return m_viewport; }

    /**
     * @brief Sets the zoom level, centered on a screen point.
     * @param zoom New zoom level (clamped to valid range).
     * @param centerScreen Screen point to keep fixed during zoom.
     */
    void setZoom(float zoom, const QPoint& centerScreen);

    /*! @brief Sets the zoom level, centered on the widget center.
     *  @param zoom New zoom level.
     */
    void setZoom(float zoom);

    /**
     * @brief Pans the viewport by the given delta.
     * @param deltaX Horizontal pan amount in widget pixels.
     * @param deltaY Vertical pan amount in widget pixels.
     */
    void pan(float deltaX, float deltaY);

    /*! @brief Resets the viewport to default (zoom 100%, centered). */
    void resetView();

    /*! @brief Fits the document in the widget viewport. */
    void fitInView();

    /*! @brief Zooms in by one step, centered on the widget. */
    void zoomIn();

    /*! @brief Zooms out by one step, centered on the widget. */
    void zoomOut();

    /*! @brief Invalidates the cached render, triggering re-render on next paint. */
    void invalidateCache();

    /**
     * @brief Replaces the active document and resets cached rendering state.
     * @param document The new document to display.
     */
    void setDocument(std::shared_ptr<Document> document);

    /*! @brief Clears the move override flag.
     *  Used when an external action (like undo) cancels a pending move.
     */
    void clearMoveOverride() { m_moveOverride = false; }

  signals:
    /*! @brief Emitted when the viewport changes (pan or zoom).
     *  @param viewport The new viewport state.
     */
    void viewportChanged(const ViewportState& viewport);

    /*! @brief Emitted when the canvas needs to be repainted. */
    void canvasModified();

    /*! @brief Emitted after a paint event completes, providing performance metrics.
     *  @param frameTimeMs The duration of the paint event in milliseconds.
     */
    void framePainted(double frameTimeMs);

  protected:
    /*! @brief Paints the rendered document with viewport transformations.
     *  @param event The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

    /*! @brief Handles mouse button press events.
     *  @param event The mouse event.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /*! @brief Handles mouse move events.
     *  @param event The mouse event.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /*! @brief Handles mouse button release events.
     *  @param event The mouse event.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /*! @brief Handles mouse wheel events for zoom and scroll.
     *  @param event The wheel event.
     */
    void wheelEvent(QWheelEvent* event) override;

    /*! @brief Handles key press events for shortcuts.
     *  @param event The key event.
     */
    void keyPressEvent(QKeyEvent* event) override;

    /*! @brief Handles key release events.
     *  @param event The key event.
     */
    void keyReleaseEvent(QKeyEvent* event) override;

    /*! @brief Handles mouse entering the widget.
     *  @param event The enter event.
     */
    void enterEvent(QEnterEvent* event) override;

    /*! @brief Handles mouse leaving the widget.
     *  @param event The leave event.
     */
    void leaveEvent(QEvent* event) override;

  private:
    /*! @brief Updates the cursor based on the active tool and state. */
    void updateCursor();

    /*! @brief Emits viewport changed signals and events. */
    void emitViewportChanged();

    /*! @brief Emits mouse position events via EventBus.
     *  @param screenPos The current screen position.
     */
    void emitMousePosition(const QPoint& screenPos) const;

    /*! @brief Dispatches mouse events to the active tool.
     *  @param event The mouse event.
     *  @param isPress True if this is a press event.
     *  @param isRelease True if this is a release event.
     */
    void dispatchToolEvent(QMouseEvent* event, bool isPress, bool isRelease);

    /*! @brief Returns the currently active tool.
     *  @return Pointer to the active tool, or nullptr.
     */
    [[nodiscard]] Tool* activeTool() const;

    /*! @brief Samples color at screen position and publishes ColorChangedEvent.
     *  Used for Alt+click color picking from any tool.
     *  @param screenPos The screen position to sample.
     */
    void sampleColorAtPosition(const QPoint& screenPos);

    /*! @brief Re-renders the document if the cache is invalid. */
    void renderIfNeeded();

    /*! @brief Updates a region of the cache directly from layer data.
     *  Faster than full re-render for interactive painting.
     */
    void updateCacheFromLayer();

    /*! @brief Updates marching ants animation for selections. */
    void advanceSelectionAnimation();

    /*! @brief Helper to draw checkerboard pattern in a given rect.
     *  @param painter The painter to draw with.
     *  @param rect The rectangle to fill with checkerboard.
     */
    void drawCheckerboard(QPainter& painter, const QRectF& rect);

    std::shared_ptr<Document> m_document;
    std::shared_ptr<SkiaRenderer> m_renderer;
    ViewportState m_viewport;

    QImage m_cachedImage;       ///< Cached rendered document image.
    bool m_cacheValid = false;  ///< Whether the cached image is valid.

    bool m_isPanning = false;
    bool m_spaceHeld = false;
    bool m_isStroking = false;    ///< True during active brush stroke.
    bool m_moveOverride = false;  ///< True when temporarily using MoveTool for Ctrl+Alt drag.
    QPoint m_lastMousePos;
    QPoint m_panStartPos;

    QTimer m_selectionTimer;
    float m_marchingOffset = 0.0F;

    QPixmap m_checkerboardTile;  ///< Cached checkerboard tile for transparency display.
};

}  // namespace gimp
