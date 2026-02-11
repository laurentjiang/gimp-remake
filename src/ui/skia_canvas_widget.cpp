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
#include "core/tools/ellipse_selection_tool.h"
#include "core/tools/move_tool.h"
#include "core/tools/rect_selection_tool.h"
#include "render/skia_renderer.h"

#include <QApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPainter>
#include <QPen>
#include <QWheelEvent>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <include/core/SkImage.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkPixmap.h>

namespace gimp {

SkiaCanvasWidget::SkiaCanvasWidget(std::shared_ptr<Document> document,
                                   std::shared_ptr<SkiaRenderer> renderer,
                                   QWidget* parent)
    : QOpenGLWidget(parent),
      m_document(std::move(document)),
      m_renderer(std::move(renderer))
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_KeyCompression, false);
    updateCursor();

    // Create checkerboard tile for transparency display (16x16 pixels, GIMP-style colors)
    constexpr int kTileSize = 16;
    m_checkerboardTile = QPixmap(kTileSize * 2, kTileSize * 2);
    m_checkerboardTile.fill(Qt::transparent);  // Initialize pixmap
    QPainter tilePainter(&m_checkerboardTile);
    tilePainter.fillRect(0, 0, kTileSize, kTileSize, QColor(102, 102, 102));          // Dark gray
    tilePainter.fillRect(kTileSize, 0, kTileSize, kTileSize, QColor(153, 153, 153));  // Light gray
    tilePainter.fillRect(0, kTileSize, kTileSize, kTileSize, QColor(153, 153, 153));  // Light gray
    tilePainter.fillRect(
        kTileSize, kTileSize, kTileSize, kTileSize, QColor(102, 102, 102));  // Dark
    tilePainter.end();

    m_selectionTimer.setInterval(80);
    connect(
        &m_selectionTimer, &QTimer::timeout, this, &SkiaCanvasWidget::advanceSelectionAnimation);
    m_selectionTimer.start();

    // Subscribe to layer events to refresh canvas when layers change
    m_layerStackSub = EventBus::instance().subscribe<LayerStackChangedEvent>(
        [this](const LayerStackChangedEvent& /*event*/) { update(); });
    m_layerSelectionSub = EventBus::instance().subscribe<LayerSelectionChangedEvent>(
        [this](const LayerSelectionChangedEvent& /*event*/) { update(); });
    m_layerPropertySub = EventBus::instance().subscribe<LayerPropertyChangedEvent>(
        [this](const LayerPropertyChangedEvent& /*event*/) { update(); });
}

SkiaCanvasWidget::~SkiaCanvasWidget()
{
    EventBus::instance().unsubscribe(m_layerStackSub);
    EventBus::instance().unsubscribe(m_layerSelectionSub);
    EventBus::instance().unsubscribe(m_layerPropertySub);
}

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
    update();
}

void SkiaCanvasWidget::initializeGL()
{
    auto gpuCtx = std::make_unique<GpuContext>();
    if (gpuCtx->initialize(context())) {
        m_gpuContext = std::move(gpuCtx);
        spdlog::info("SkiaCanvasWidget: GPU context initialized successfully");
    } else {
        spdlog::warn("SkiaCanvasWidget: GPU context init failed, falling back to raster");
        m_gpuContext = std::make_unique<NullGpuContext>();
    }
    m_renderer->setGpuContext(m_gpuContext.get());
}

void SkiaCanvasWidget::resizeGL(int /*w*/, int /*h*/)
{
    // Surfaces will be recreated on next render - nothing to do here
}

void SkiaCanvasWidget::paintGL()
{
    const auto startTime = std::chrono::high_resolution_clock::now();

    if (!m_document || !m_renderer) {
        return;
    }

    // Ensure GPU context is valid (should always be after initializeGL)
    if (!m_gpuContext) {
        spdlog::error("SkiaCanvasWidget::paintGL called with null GPU context");
        return;
    }

    // 1. Render document via Skia (GPU or CPU based on context)
    m_renderer->render(*m_document);

    // 2. Copy rendered surface to QImage BEFORE resetting GL state
    //    GPU readPixels requires valid Skia GL state
    QImage renderImage;
    auto skImage = m_renderer->get_result();
    if (skImage) {
        const int imgW = skImage->width();
        const int imgH = skImage->height();

        // Create QImage with correct format for Windows (BGRA)
        renderImage = QImage(imgW, imgH, QImage::Format_ARGB32_Premultiplied);

        // Use BGRA format for SkPixmap to match QImage's byte order on Windows
        const SkImageInfo targetInfo =
            SkImageInfo::Make(imgW, imgH, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
        const SkPixmap pixmap(targetInfo, renderImage.bits(), renderImage.bytesPerLine());

        if (!skImage->readPixels(pixmap, 0, 0)) {
            spdlog::warn("SkiaCanvasWidget: readPixels failed");
            renderImage = QImage();  // Clear on failure
        }
    }

    // 3. Flush Skia GPU work
    m_gpuContext->flush();

    // 4. CRITICAL: Reset GL state so Qt's QPainter works correctly
    //    Skia leaves bound textures/shaders that confuse Qt
    m_gpuContext->resetContext();

    // 5. Draw overlays using QPainter (after GL state reset)
    QPainter painter(this);
    painter.fillRect(rect(), QColor(64, 64, 64));

    const QRectF targetRect(m_viewport.panX,
                            m_viewport.panY,
                            static_cast<float>(m_document->width()) * m_viewport.zoomLevel,
                            static_cast<float>(m_document->height()) * m_viewport.zoomLevel);

    // Draw checkerboard pattern for transparency visualization
    drawCheckerboard(painter, targetRect);

    // Draw the pre-rendered document image
    if (!renderImage.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, m_viewport.zoomLevel < 1.0F);
        painter.drawImage(targetRect, renderImage);
    }

    // Draw pixel grid at high zoom
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

    // Render floating buffer for selection move preview
    // Always check MoveTool for active floating selection, regardless of current tool
    auto* moveTool = dynamic_cast<MoveTool*>(ToolFactory::instance().getTool("move"));
    if (moveTool && moveTool->isMovingSelection()) {
        QRect floatBounds = moveTool->floatingRect();
        QPoint floatOffset = moveTool->floatingOffset();
        QSizeF floatScale = moveTool->floatingScale();

        if (!floatBounds.isEmpty()) {
            // Clip to document bounds - floating buffer should appear behind gray background
            painter.save();
            painter.setClipRect(targetRect);

            // Get scaled buffer if scaling, otherwise use original
            std::vector<std::uint8_t> scaledBuf;
            const std::uint8_t* bufData = nullptr;
            int bufWidth = 0;
            int bufHeight = 0;

            if (moveTool->isScaling() || std::abs(floatScale.width() - 1.0) > 0.001 ||
                std::abs(floatScale.height() - 1.0) > 0.001) {
                scaledBuf = moveTool->getScaledBuffer();
                QSize scaledSize = moveTool->getScaledSize();
                bufData = scaledBuf.data();
                bufWidth = scaledSize.width();
                bufHeight = scaledSize.height();
            } else {
                const auto* floatBuf = moveTool->floatingBuffer();
                if (floatBuf && !floatBuf->empty()) {
                    bufData = floatBuf->data();
                    bufWidth = floatBounds.width();
                    bufHeight = floatBounds.height();
                }
            }

            if (bufData && bufWidth > 0 && bufHeight > 0) {
                // Create QImage from buffer (RGBA format)
                QImage floatingImage(
                    bufData, bufWidth, bufHeight, bufWidth * 4, QImage::Format_RGBA8888);

                // Calculate destination rect with offset applied
                float destX =
                    m_viewport.panX +
                    static_cast<float>(floatBounds.x() + floatOffset.x()) * m_viewport.zoomLevel;
                float destY =
                    m_viewport.panY +
                    static_cast<float>(floatBounds.y() + floatOffset.y()) * m_viewport.zoomLevel;
                float destW = static_cast<float>(bufWidth) * m_viewport.zoomLevel;
                float destH = static_cast<float>(bufHeight) * m_viewport.zoomLevel;

                QRectF floatingRect(destX, destY, destW, destH);
                painter.drawImage(floatingRect, floatingImage);
            }

            painter.restore();
        }
    }

    const QPainterPath selectionPath = SelectionManager::instance().displayPath();
    if (!selectionPath.isEmpty()) {
        painter.save();

        // Clip marching ants to document bounds - selection should appear behind gray background
        painter.setClipRect(targetRect);

        QTransform transform;
        transform.translate(m_viewport.panX, m_viewport.panY);
        transform.scale(m_viewport.zoomLevel, m_viewport.zoomLevel);

        // If moving/scaling selection contents, apply the transform
        if (moveTool && moveTool->isMovingSelection()) {
            QPoint moveOffset = moveTool->floatingOffset();
            QSizeF scale = moveTool->floatingScale();
            // Use full selection bounds (not clipped sourceRect) for marching ants
            QRectF bounds = moveTool->selectionBounds();

            // Transform: translate to position, scale from top-left
            transform.translate(bounds.x() + moveOffset.x(), bounds.y() + moveOffset.y());
            transform.scale(scale.width(), scale.height());
            transform.translate(-bounds.x(), -bounds.y());
        }

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

    // Draw transform handles:
    // - When MoveTool has floating buffer: use its getHandleRects() (for content transform)
    // - When selection tool is in Adjusting phase: use its getHandleRects() (for outline resize)
    std::vector<QRectF> handleRectsF;
    if (moveTool && moveTool->isMovingSelection()) {
        // MoveTool handles for floating buffer transform
        for (const auto& r : moveTool->getHandleRects()) {
            handleRectsF.emplace_back(r);
        }
    } else {
        // Check if current tool is a selection tool in Adjusting phase
        Tool* currentTool = activeTool();
        if (auto* rectTool = dynamic_cast<RectSelectTool*>(currentTool)) {
            if (rectTool->phase() == SelectionPhase::Adjusting) {
                auto handles = rectTool->getHandleRects(m_viewport.zoomLevel);
                handleRectsF.assign(handles.begin(), handles.end());
            }
        } else if (auto* ellipseTool = dynamic_cast<EllipseSelectTool*>(currentTool)) {
            if (ellipseTool->phase() == EllipseSelectionPhase::Adjusting) {
                auto handles = ellipseTool->getHandleRects(m_viewport.zoomLevel);
                handleRectsF.assign(handles.begin(), handles.end());
            }
        }
    }

    if (!handleRectsF.empty()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);

        // Transform handle rects to screen coordinates
        for (const auto& handleRect : handleRectsF) {
            float screenX =
                m_viewport.panX + static_cast<float>(handleRect.x()) * m_viewport.zoomLevel;
            float screenY =
                m_viewport.panY + static_cast<float>(handleRect.y()) * m_viewport.zoomLevel;
            float screenW = static_cast<float>(handleRect.width()) * m_viewport.zoomLevel;
            float screenH = static_cast<float>(handleRect.height()) * m_viewport.zoomLevel;

            // Ensure minimum visible size
            screenW = std::max(6.0F, screenW);
            screenH = std::max(6.0F, screenH);

            QRectF screenRect(screenX, screenY, screenW, screenH);

            // Draw handle: white fill with black border
            painter.setPen(QPen(Qt::black, 1));
            painter.setBrush(Qt::white);
            painter.drawRect(screenRect);
        }

        painter.restore();
    }

    painter.end();

    // 5. CRITICAL: Reset state AGAIN so Skia works next frame
    //    Qt leaves the GL state dirty
    m_gpuContext->resetContext();

    const auto endTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> frameDuration = endTime - startTime;
    emit framePainted(frameDuration.count());
}

void SkiaCanvasWidget::drawCheckerboard(QPainter& painter, const QRectF& rect)
{
    // Clip to the visible intersection of the canvas rect and widget
    QRectF visibleRect = rect.intersected(QRectF(this->rect()));
    if (visibleRect.isEmpty()) {
        return;
    }

    // Save painter state
    painter.save();
    painter.setClipRect(visibleRect);

    // Calculate tile size scaled by zoom
    constexpr int kBaseTileSize = 16;
    int scaledTileSize = std::max(
        8, static_cast<int>(kBaseTileSize * m_viewport.zoomLevel));  // Min 8 for visibility

    // Scale the checkerboard tile to match zoom (use FastTransformation to preserve hard edges)
    QPixmap scaledTile = m_checkerboardTile.scaled(
        scaledTileSize * 2, scaledTileSize * 2, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // Draw tiled pixmap from the canvas origin (rect.topLeft()) so pattern aligns with canvas
    // The clip rect ensures we only paint within the visible area
    painter.drawTiledPixmap(rect.toRect(), scaledTile);

    painter.restore();
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
    // Forward to MoveTool if there's an active floating selection (move override or active float)
    auto* moveTool = dynamic_cast<MoveTool*>(ToolFactory::instance().getTool("move"));
    if (moveTool && moveTool->isMovingSelection()) {
        if (moveTool->onKeyPress(static_cast<Qt::Key>(event->key()), event->modifiers())) {
            // If floating buffer was committed, clear move override and refresh
            if (!moveTool->isMovingSelection()) {
                m_moveOverride = false;
                invalidateCache();
                emit canvasModified();
            }
            event->accept();
            update();
            return;
        }
    }

    // Forward to active tool if in Active state
    Tool* tool = activeTool();
    if (tool && tool->state() == ToolState::Active) {
        if (tool->onKeyPress(static_cast<Qt::Key>(event->key()), event->modifiers())) {
            event->accept();
            return;
        }
    }

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
    // Forward to active tool if in Active state
    Tool* tool = activeTool();
    if (tool && tool->state() == ToolState::Active) {
        if (tool->onKeyRelease(static_cast<Qt::Key>(event->key()), event->modifiers())) {
            event->accept();
            return;
        }
    }

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
    toolEvent.zoomLevel = m_viewport.zoomLevel;

    // If move override is active, route ALL events to MoveTool (including press for handles)
    if (m_moveOverride) {
        auto* moveTool = dynamic_cast<MoveTool*>(ToolFactory::instance().getTool("move"));
        if (moveTool) {
            bool handled = false;
            if (isPress) {
                m_isStroking = true;
                handled = moveTool->onMousePress(toolEvent);
                if (handled) {
                    update();
                }
            } else if (isRelease) {
                handled = moveTool->onMouseRelease(toolEvent);
                m_isStroking = false;
                // Only clear move override if the move actually committed
                if (!moveTool->isMovingSelection()) {
                    m_moveOverride = false;
                }
                if (handled) {
                    invalidateCache();
                    emit canvasModified();
                }
            } else {
                handled = moveTool->onMouseMove(toolEvent);
                if (handled || moveTool->isMovingSelection()) {
                    update();
                }
            }
            return;
        }
    }

    // Check for Ctrl+Alt move override on press
    if (isPress) {
        // Auto-commit any pending move operation before starting new stroke,
        // but only if clicking OUTSIDE the transformed selection bounds.
        // Clicking inside the selection continues dragging; clicking on handles scales.
        auto* pendingMoveTool = dynamic_cast<MoveTool*>(ToolFactory::instance().getTool("move"));
        if (pendingMoveTool && pendingMoveTool->isMovingSelection()) {
            QRectF txBounds = pendingMoveTool->transformedBounds();
            bool insideBounds = txBounds.contains(QPointF(toolEvent.canvasPos));
            bool onHandle = pendingMoveTool->hitTestHandle(
                                toolEvent.canvasPos, toolEvent.zoomLevel) != TransformHandle::None;

            // Only commit if clicking outside the selection AND not on a handle
            if (!insideBounds && !onHandle) {
                pendingMoveTool->commitFloatingBuffer();
                invalidateCache();
            }
        }
        m_moveOverride = false;  // Reset on new stroke

        const bool ctrlAlt = (event->modifiers() & Qt::ControlModifier) != 0 &&
                             (event->modifiers() & Qt::AltModifier) != 0;
        const bool shiftAlt = (event->modifiers() & Qt::ShiftModifier) != 0 &&
                              (event->modifiers() & Qt::AltModifier) != 0;
        const std::string& activeToolId = ToolRegistry::instance().getActiveTool();
        const bool isSelectionTool = activeToolId.find("select") != std::string::npos;

        // Ctrl+Alt = cut-move, Shift+Alt = copy-move
        if ((ctrlAlt || shiftAlt) && isSelectionTool &&
            SelectionManager::instance().hasSelection()) {
            const QPainterPath& selPath = SelectionManager::instance().selectionPath();
            if (selPath.contains(QPointF(toolEvent.canvasPos))) {
                // Click inside selection with modifier - delegate to MoveTool
                auto* moveTool = dynamic_cast<MoveTool*>(ToolFactory::instance().getTool("move"));
                if (moveTool) {
                    // Reset the current selection tool to Idle so it doesn't have stale state
                    // when we return to it later. The reset() method calls cancelStroke internally.
                    if (auto* rectTool = dynamic_cast<RectSelectTool*>(tool)) {
                        rectTool->resetToIdle();
                    } else if (auto* ellipseTool = dynamic_cast<EllipseSelectTool*>(tool)) {
                        ellipseTool->resetToIdle();
                    }

                    // Reset MoveTool to ensure it's in Idle state before starting
                    moveTool->reset();
                    moveTool->setCopyMode(shiftAlt);  // Shift+Alt = copy mode
                    m_moveOverride = true;
                    m_isStroking = true;
                    bool handled = moveTool->onMousePress(toolEvent);
                    if (handled) {
                        update();
                        emit canvasModified();
                    }
                    return;
                }
            }
        }
    }

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
        // GPU rendering: just trigger repaint
        update();
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

    auto layer = m_document->activeLayer();
    if (!layer) {
        return;
    }
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
