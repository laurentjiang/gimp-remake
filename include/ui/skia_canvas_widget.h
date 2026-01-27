/**
 * @file skia_canvas_widget.h
 * @brief Interactive canvas widget with pan, zoom, and tool input handling.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include <QPointF>
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

  signals:
    /*! @brief Emitted when the viewport changes (pan or zoom).
     *  @param viewport The new viewport state.
     */
    void viewportChanged(const ViewportState& viewport);

    /*! @brief Emitted when the canvas needs to be repainted. */
    void canvasModified();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    void updateCursor();
    void emitViewportChanged();
    void emitMousePosition(const QPoint& screenPos) const;
    void dispatchToolEvent(QMouseEvent* event, bool isPress, bool isRelease);
    [[nodiscard]] Tool* activeTool() const;

    std::shared_ptr<Document> m_document;
    std::shared_ptr<SkiaRenderer> m_renderer;
    ViewportState m_viewport;

    bool m_isPanning = false;
    bool m_spaceHeld = false;
    QPoint m_lastMousePos;
    QPoint m_panStartPos;
};

}  // namespace gimp
