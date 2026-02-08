/**
 * @file skia_canvas_widget.cpp
 * @brief Implementation of SkiaCanvasWidget with interactive pan, zoom, and tool input.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "ui/skia_canvas_widget.h"

#include "core/document.h"
#include "core/event_bus.h"
#include "core/events.h"
#include "core/layer.h"
#include "core/selection_manager.h"
#include "core/tool.h"
#include "core/tool_factory.h"
#include "core/tool_registry.h"
#include "render/skia_renderer.h"

#include <QApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <include/core/SkImage.h>
#include <include/core/SkPixmap.h>

namespace gimp {

SkiaCanvasWidget::SkiaCanvasWidget(std::shared_ptr<Document> document,
                                   std::shared_ptr<SkiaRenderer> renderer,
                                   QWidget* parent)
    : QWidget(parent),
      m_document(std::move(document)),
      m_renderer(std::move(renderer))
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_KeyCompression, false);
    updateCursor();
    invalidateCache();

    m_selectionTimer.setInterval(80);
    connect(
        &m_selectionTimer, &QTimer::timeout, this, &SkiaCanvasWidget::advanceSelectionAnimation);
    m_selectionTimer.start();
}

SkiaCanvasWidget::~SkiaCanvasWidget() = default;

QPointF SkiaCanvasWidget::screenToCanvas(const QPoint& screenPos) const
{
    const float x = (static_cast<float>(screenPos.x()) - m_viewport.panX) / m_viewport.zoomLevel;
    const float y = (static_cast<float>(screenPos.y()) - m_viewport.panY) / m_viewport.zoomLevel;
    return {x, y};
}

QPoint SkiaCanvasWidget::canvasToScreen(const QPointF& canvasPos) const
{
    const int x =
        static_cast<int>(std::round((canvasPos.x() * m_viewport.zoomLevel) + m_viewport.panX));
    const int y =
        static_cast<int>(std::round((canvasPos.y() * m_viewport.zoomLevel) + m_viewport.panY));
    return {x, y};
}

void SkiaCanvasWidget::setZoom(float zoom, const QPoint& centerScreen)
{
    const float newZoom = std::clamp(zoom, ViewportState::MIN_ZOOM, ViewportState::MAX_ZOOM);

    if (std::abs(newZoom - m_viewport.zoomLevel) < 0.0001F) {
        return;
    }

    const QPointF canvasPoint = screenToCanvas(centerScreen);

    m_viewport.zoomLevel = newZoom;

    const QPoint newScreen = canvasToScreen(canvasPoint);
    m_viewport.panX += static_cast<float>(centerScreen.x() - newScreen.x());
    m_viewport.panY += static_cast<float>(centerScreen.y() - newScreen.y());

    emitViewportChanged();
    update();
}

void SkiaCanvasWidget::setZoom(float zoom)
{
    setZoom(zoom, QPoint(width() / 2, height() / 2));
}

void SkiaCanvasWidget::pan(float deltaX, float deltaY)
{
    m_viewport.panX += deltaX;
    m_viewport.panY += deltaY;
    emitViewportChanged();
    update();
}

void SkiaCanvasWidget::resetView()
{
    m_viewport.zoomLevel = 1.0F;
    m_viewport.panX = 0.0F;
    m_viewport.panY = 0.0F;
    emitViewportChanged();
    update();
}

void SkiaCanvasWidget::fitInView()
{
    if (!m_document) {
        return;
    }

    const float docWidth = static_cast<float>(m_document->width());
    const float docHeight = static_cast<float>(m_document->height());
    const float widgetWidth = static_cast<float>(width());
    const float widgetHeight = static_cast<float>(height());

    const float scaleX = widgetWidth / docWidth;
    const float scaleY = widgetHeight / docHeight;
    const float newZoom = std::clamp(
        std::min(scaleX, scaleY) * 0.9F, ViewportState::MIN_ZOOM, ViewportState::MAX_ZOOM);

    m_viewport.zoomLevel = newZoom;
    m_viewport.panX = (widgetWidth - (docWidth * newZoom)) / 2.0F;
    m_viewport.panY = (widgetHeight - (docHeight * newZoom)) / 2.0F;

    emitViewportChanged();
    update();
}

void SkiaCanvasWidget::zoomIn()
{
    setZoom(m_viewport.zoomLevel * ViewportState::ZOOM_STEP);
}

void SkiaCanvasWidget::zoomOut()
{
    setZoom(m_viewport.zoomLevel / ViewportState::ZOOM_STEP);
}

void SkiaCanvasWidget::invalidateCache()
{
    m_cacheValid = false;
    update();
}

void SkiaCanvasWidget::renderIfNeeded()
{
    if (m_cacheValid || !m_document || !m_renderer) {
        return;
    }

    m_renderer->render(*m_document);

    auto skImage = m_renderer->get_result();
    if (!skImage) {
        return;
    }

    const SkImageInfo info = skImage->imageInfo();
    m_cachedImage = QImage(info.width(), info.height(), QImage::Format_ARGB32_Premultiplied);

    const SkPixmap pixmap(info, m_cachedImage.bits(), m_cachedImage.bytesPerLine());
    if (skImage->readPixels(pixmap, 0, 0)) {
        m_cacheValid = true;
    }
}

void SkiaCanvasWidget::updateCacheFromLayer()
{
    if (!m_document || m_document->layers().count() == 0) {
        return;
    }

    auto layer = m_document->layers()[0];
    const int w = layer->width();
    const int h = layer->height();

    if (m_cachedImage.isNull() || m_cachedImage.width() != w || m_cachedImage.height() != h) {
        m_cachedImage = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    }

    // Convert from RGBA (layer storage) to BGRA (QImage Format_ARGB32 on little-endian)
    const auto& layerData = layer->data();
    if (layerData.size() == static_cast<size_t>(w) * static_cast<size_t>(h) * 4U) {
        std::uint8_t* dest = m_cachedImage.bits();
        const std::uint8_t* src = layerData.data();
        const size_t pixelCount = static_cast<size_t>(w) * static_cast<size_t>(h);

        for (size_t i = 0; i < pixelCount; ++i) {
            // RGBA -> BGRA: swap R and B channels
            dest[i * 4 + 0] = src[i * 4 + 2];  // B <- B from src
            dest[i * 4 + 1] = src[i * 4 + 1];  // G <- G
            dest[i * 4 + 2] = src[i * 4 + 0];  // R <- R from src
            dest[i * 4 + 3] = src[i * 4 + 3];  // A <- A
        }
        m_cacheValid = true;
    }
}

void SkiaCanvasWidget::paintEvent(QPaintEvent* event)
{
    (void)event;

    QPainter painter(this);
    painter.fillRect(rect(), QColor(64, 64, 64));

    if (!m_document || !m_renderer) {
        return;
    }

    renderIfNeeded();

    if (!m_cacheValid || m_cachedImage.isNull()) {
        return;
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_viewport.zoomLevel < 1.0F);

    const QRectF targetRect(m_viewport.panX,
                            m_viewport.panY,
                            static_cast<float>(m_cachedImage.width()) * m_viewport.zoomLevel,
                            static_cast<float>(m_cachedImage.height()) * m_viewport.zoomLevel);
    painter.drawImage(targetRect, m_cachedImage);

    if (m_viewport.zoomLevel >= 8.0F) {
        painter.setPen(QColor(128, 128, 128, 80));
        const int startX = static_cast<int>(m_viewport.panX);
        const int startY = static_cast<int>(m_viewport.panY);
        const int step = static_cast<int>(m_viewport.zoomLevel);

        for (int x = startX; x < width(); x += step) {
            if (x >= 0) {
                painter.drawLine(x, 0, x, height());
            }
        }
        for (int y = startY; y < height(); y += step) {
            if (y >= 0) {
                painter.drawLine(0, y, width(), y);
            }
        }
    }

    const QPainterPath selectionPath = SelectionManager::instance().displayPath();
    if (!selectionPath.isEmpty()) {
        painter.save();

        QTransform transform;
        transform.translate(m_viewport.panX, m_viewport.panY);
        transform.scale(m_viewport.zoomLevel, m_viewport.zoomLevel);
        painter.setTransform(transform, true);
        painter.setRenderHint(QPainter::Antialiasing, true);

        constexpr qreal dashLength = 4.0;
        const QVector<qreal> dashPattern = {dashLength, dashLength};

        QPen whitePen(QColor(240, 240, 240));
        whitePen.setCosmetic(true);
        whitePen.setWidth(1);
        whitePen.setDashPattern(dashPattern);
        whitePen.setDashOffset(m_marchingOffset);
        whitePen.setCapStyle(Qt::FlatCap);

        QPen blackPen(QColor(16, 16, 16));
        blackPen.setCosmetic(true);
        blackPen.setWidth(1);
        blackPen.setDashPattern(dashPattern);
        blackPen.setDashOffset(m_marchingOffset + dashLength);
        blackPen.setCapStyle(Qt::FlatCap);

        painter.setPen(whitePen);
        painter.drawPath(selectionPath);

        painter.setPen(blackPen);
        painter.drawPath(selectionPath);

        painter.restore();
    }
}

void SkiaCanvasWidget::advanceSelectionAnimation()
{
    if (!SelectionManager::instance().hasSelection() &&
        !SelectionManager::instance().hasPreview()) {
        return;
    }

    m_marchingOffset += 1.0F;
    if (m_marchingOffset >= 8.0F) {
        m_marchingOffset = 0.0F;
    }
    update();
}

void SkiaCanvasWidget::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();

    if ((event->button() == Qt::MiddleButton) ||
        (m_spaceHeld && event->button() == Qt::LeftButton)) {
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // Alt+click to pick color from any tool (except selection tools that use Alt for center-out)
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier) != 0) {
        const std::string& activeToolId = ToolRegistry::instance().getActiveTool();
        if (activeToolId.find("select") == std::string::npos) {
            sampleColorAtPosition(event->pos());
            return;
        }
    }

    dispatchToolEvent(event, true, false);
}

void SkiaCanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint currentPos = event->pos();

    emitMousePosition(currentPos);

    if (m_isPanning) {
        const int dx = currentPos.x() - m_lastMousePos.x();
        const int dy = currentPos.y() - m_lastMousePos.y();
        pan(static_cast<float>(dx), static_cast<float>(dy));
        m_lastMousePos = currentPos;
        return;
    }

    dispatchToolEvent(event, false, false);
    m_lastMousePos = currentPos;
}

void SkiaCanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isPanning && ((event->button() == Qt::MiddleButton) ||
                        (m_spaceHeld && event->button() == Qt::LeftButton))) {
        m_isPanning = false;
        updateCursor();
        return;
    }

    dispatchToolEvent(event, false, true);
}

void SkiaCanvasWidget::wheelEvent(QWheelEvent* event)
{
    const QPoint angleDelta = event->angleDelta();

    if (event->modifiers() & Qt::ControlModifier) {
        const float zoomFactor =
            (angleDelta.y() > 0) ? ViewportState::ZOOM_STEP : (1.0F / ViewportState::ZOOM_STEP);
        setZoom(m_viewport.zoomLevel * zoomFactor, event->position().toPoint());
    } else {
        const float scrollSpeed = 40.0F;
        if (event->modifiers() & Qt::ShiftModifier) {
            pan(static_cast<float>(angleDelta.y()) / 120.0F * scrollSpeed, 0.0F);
        } else {
            pan(static_cast<float>(angleDelta.x()) / 120.0F * scrollSpeed,
                static_cast<float>(angleDelta.y()) / 120.0F * scrollSpeed);
        }
    }

    event->accept();
}

void SkiaCanvasWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = true;
        if (!m_isPanning) {
            setCursor(Qt::OpenHandCursor);
        }
        return;
    }

    if (event->key() == Qt::Key_0 && (event->modifiers() & Qt::ControlModifier)) {
        resetView();
        return;
    }

    if (event->key() == Qt::Key_1 && (event->modifiers() & Qt::ControlModifier)) {
        setZoom(1.0F);
        return;
    }

    if ((event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) &&
        (event->modifiers() & Qt::ControlModifier)) {
        zoomIn();
        return;
    }

    if (event->key() == Qt::Key_Minus && (event->modifiers() & Qt::ControlModifier)) {
        zoomOut();
        return;
    }

    QWidget::keyPressEvent(event);
}

void SkiaCanvasWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = false;
        if (!m_isPanning) {
            updateCursor();
        }
        return;
    }

    QWidget::keyReleaseEvent(event);
}

void SkiaCanvasWidget::enterEvent(QEnterEvent* event)
{
    (void)event;
    setFocus();
    updateCursor();
}

void SkiaCanvasWidget::leaveEvent(QEvent* event)
{
    (void)event;
    EventBus::instance().publish(MousePositionChangedEvent{-1, -1, -1, -1});
}

void SkiaCanvasWidget::updateCursor()
{
    if (m_spaceHeld) {
        setCursor(Qt::OpenHandCursor);
        return;
    }

    const std::string& activeToolId = ToolRegistry::instance().getActiveTool();

    if (activeToolId == "zoom") {
        setCursor(Qt::SizeFDiagCursor);
    } else if (activeToolId == "move") {
        setCursor(Qt::SizeAllCursor);
    } else if (activeToolId == "color_picker") {
        setCursor(Qt::CrossCursor);
    } else if (activeToolId == "text") {
        setCursor(Qt::IBeamCursor);
    } else if (activeToolId.find("select") != std::string::npos) {
        setCursor(Qt::CrossCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void SkiaCanvasWidget::emitViewportChanged()
{
    emit viewportChanged(m_viewport);

    CanvasViewChangedEvent event;
    event.zoomLevel = m_viewport.zoomLevel;
    event.panX = m_viewport.panX;
    event.panY = m_viewport.panY;
    EventBus::instance().publish(event);
}

void SkiaCanvasWidget::emitMousePosition(const QPoint& screenPos) const
{
    const QPointF canvasPos = screenToCanvas(screenPos);

    MousePositionChangedEvent event;
    event.canvasX = static_cast<int>(std::floor(canvasPos.x()));
    event.canvasY = static_cast<int>(std::floor(canvasPos.y()));
    event.screenX = screenPos.x();
    event.screenY = screenPos.y();
    EventBus::instance().publish(event);
}

void SkiaCanvasWidget::dispatchToolEvent(QMouseEvent* event, bool isPress, bool isRelease)
{
    Tool* tool = activeTool();
    if (!tool) {
        return;
    }

    const QPointF canvasPos = screenToCanvas(event->pos());

    ToolInputEvent toolEvent;
    toolEvent.canvasPos = QPoint(static_cast<int>(std::floor(canvasPos.x())),
                                 static_cast<int>(std::floor(canvasPos.y())));
    toolEvent.screenPos = event->pos();
    toolEvent.buttons = event->buttons();
    toolEvent.modifiers = event->modifiers();
    toolEvent.pressure = 1.0F;

    bool handled = false;
    if (isPress) {
        m_isStroking = true;
        handled = tool->onMousePress(toolEvent);
    } else if (isRelease) {
        handled = tool->onMouseRelease(toolEvent);
        m_isStroking = false;
        // Full re-render on stroke end for proper compositing
        if (handled) {
            invalidateCache();
            emit canvasModified();
        }
        return;
    } else {
        handled = tool->onMouseMove(toolEvent);
    }

    if (handled) {
        // Fast path: update cache directly from layer during stroke
        if (m_isStroking) {
            updateCacheFromLayer();
            update();
        } else {
            invalidateCache();
        }
        emit canvasModified();
    }
}

Tool* SkiaCanvasWidget::activeTool() const
{
    return ToolFactory::instance().activeTool();
}

void SkiaCanvasWidget::sampleColorAtPosition(const QPoint& screenPos)
{
    if (!m_document || m_document->layers().count() == 0) {
        return;
    }

    const QPointF canvasPos = screenToCanvas(screenPos);
    const int x = static_cast<int>(std::floor(canvasPos.x()));
    const int y = static_cast<int>(std::floor(canvasPos.y()));

    auto layer = m_document->layers()[0];
    const int width = layer->width();
    const int height = layer->height();

    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }

    const auto& layerData = layer->data();
    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x)) *
                               4;

    const std::uint8_t r = layerData[offset + 0];
    const std::uint8_t g = layerData[offset + 1];
    const std::uint8_t b = layerData[offset + 2];
    const std::uint8_t a = layerData[offset + 3];

    const std::uint32_t color =
        (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) |
        (static_cast<std::uint32_t>(b) << 8) | static_cast<std::uint32_t>(a);

    ColorChangedEvent event;
    event.color = color;
    event.source = "alt_click";
    EventBus::instance().publish(event);
}

}  // namespace gimp
