/**
 * @file skia_canvas_widget.cpp
 * @brief Implementation of SkiaCanvasWidget.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "ui/skia_canvas_widget.h"

#include "core/document.h"
#include "render/skia_renderer.h"

#include <QImage>
#include <QPainter>

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
}

SkiaCanvasWidget::~SkiaCanvasWidget() = default;

void SkiaCanvasWidget::paintEvent(QPaintEvent* event)
{
    (void)event;

    if (!m_document || !m_renderer) {
        return;
    }

    m_renderer->render(*m_document);

    auto skImage = m_renderer->get_result();
    if (!skImage) {
        return;
    }

    const SkImageInfo info = skImage->imageInfo();
    QImage qImage(info.width(), info.height(), QImage::Format_ARGB32_Premultiplied);

    const SkPixmap pixmap(info, qImage.bits(), qImage.bytesPerLine());
    if (skImage->readPixels(pixmap, 0, 0)) {
        QPainter painter(this);
        painter.drawImage(0, 0, qImage);
    }
}

}  // namespace gimp
