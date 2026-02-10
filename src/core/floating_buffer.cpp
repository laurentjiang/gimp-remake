/**
 * @file floating_buffer.cpp
 * @brief Implementation of FloatingBuffer.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "core/floating_buffer.h"

#include "core/layer.h"
#include "core/selection_manager.h"

#include <QImage>
#include <QPainter>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gimp {

void FloatingBuffer::clear()
{
    buffer_.clear();
    sourceRect_ = QRect();
    selectionMask_.clear();
}

bool FloatingBuffer::extractFromLayer(const std::shared_ptr<Layer>& layer,
                                      const QPainterPath& selectionPath)
{
    if (!layer || selectionPath.isEmpty()) {
        spdlog::warn("[FloatingBuffer] extractFromLayer: invalid layer or empty selection");
        return false;
    }

    QRectF boundingF = selectionPath.boundingRect();
    QRect bounding = boundingF.toAlignedRect();

    spdlog::debug("[FloatingBuffer] Selection bounds: ({},{}) {}x{}, layer: {}x{}",
                  bounding.left(),
                  bounding.top(),
                  bounding.width(),
                  bounding.height(),
                  layer->width(),
                  layer->height());

    // Clip to layer bounds
    int x1 = std::max(0, bounding.left());
    int y1 = std::max(0, bounding.top());
    int x2 = std::min(layer->width(), bounding.right() + 1);
    int y2 = std::min(layer->height(), bounding.bottom() + 1);

    if (x2 <= x1 || y2 <= y1) {
        spdlog::warn(
            "[FloatingBuffer] Selection completely outside layer: clipped=({},{}) to ({},{})",
            x1,
            y1,
            x2,
            y2);
        return false;
    }

    int width = x2 - x1;
    int height = y2 - y1;
    sourceRect_ = QRect(x1, y1, width, height);

    spdlog::debug("[FloatingBuffer] Extracting {}x{} pixels at ({},{})", width, height, x1, y1);

    // Pre-rasterize the selection mask
    rasterizeSelectionMask(selectionPath, sourceRect_);

    // Allocate buffer (RGBA, 4 bytes per pixel) - initialize to transparent
    buffer_.resize(static_cast<std::size_t>(width * height) * 4, 0);

    const auto& layerData = layer->data();
    int layerWidth = layer->width();
    constexpr int kPixelSize = 4;

    // Copy pixels that are inside the selection
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (isPixelSelected(col, row)) {
                int px = x1 + col;
                int py = y1 + row;
                std::size_t srcOffset =
                    (static_cast<std::size_t>(py) * layerWidth + px) * kPixelSize;
                std::size_t dstOffset = (static_cast<std::size_t>(row) * width + col) * kPixelSize;

                std::memcpy(buffer_.data() + dstOffset, layerData.data() + srcOffset, kPixelSize);
            }
        }
    }

    return true;
}

void FloatingBuffer::clearSourcePixels(const std::shared_ptr<Layer>& layer)
{
    if (!layer || sourceRect_.isEmpty()) {
        return;
    }

    auto& layerData = layer->data();
    int layerWidth = layer->width();
    constexpr int kPixelSize = 4;

    int x1 = sourceRect_.left();
    int y1 = sourceRect_.top();
    int width = sourceRect_.width();
    int height = sourceRect_.height();

    // Clear pixels inside selection to transparent
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (isPixelSelected(col, row)) {
                int px = x1 + col;
                int py = y1 + row;
                std::size_t offset = (static_cast<std::size_t>(py) * layerWidth + px) * kPixelSize;
                layerData[offset + 0] = 0;  // R
                layerData[offset + 1] = 0;  // G
                layerData[offset + 2] = 0;  // B
                layerData[offset + 3] = 0;  // A
            }
        }
    }
}

void FloatingBuffer::pasteToLayer(const std::shared_ptr<Layer>& layer, QPoint offset)
{
    if (!layer || buffer_.empty()) {
        return;
    }

    auto& layerData = layer->data();
    int layerWidth = layer->width();
    int layerHeight = layer->height();
    constexpr int kPixelSize = 4;

    int width = sourceRect_.width();
    int height = sourceRect_.height();
    int x1 = sourceRect_.left();
    int y1 = sourceRect_.top();

    // Paste pixels (only those inside the original selection mask)
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (!isPixelSelected(col, row)) {
                continue;
            }

            int destPx = x1 + col + offset.x();
            int destPy = y1 + row + offset.y();

            // Clip to layer bounds
            if (destPx < 0 || destPx >= layerWidth || destPy < 0 || destPy >= layerHeight) {
                continue;
            }

            std::size_t srcOffset = (static_cast<std::size_t>(row) * width + col) * kPixelSize;
            std::size_t dstOffset =
                (static_cast<std::size_t>(destPy) * layerWidth + destPx) * kPixelSize;

            std::memcpy(layerData.data() + dstOffset, buffer_.data() + srcOffset, kPixelSize);
        }
    }
}

std::vector<std::uint8_t> FloatingBuffer::getScaled(QSizeF scale) const
{
    if (buffer_.empty()) {
        return {};
    }

    int srcW = sourceRect_.width();
    int srcH = sourceRect_.height();
    int dstW = static_cast<int>(std::round(srcW * scale.width()));
    int dstH = static_cast<int>(std::round(srcH * scale.height()));

    if (dstW <= 0 || dstH <= 0) {
        return {};
    }

    // Use Qt for high-quality scaling
    QImage srcImage(buffer_.data(), srcW, srcH, srcW * 4, QImage::Format_RGBA8888);
    QImage dstImage = srcImage.scaled(dstW, dstH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Copy to vector
    std::vector<std::uint8_t> result(static_cast<std::size_t>(dstW * dstH) * 4);
    for (int row = 0; row < dstH; ++row) {
        const uchar* scanline = dstImage.constScanLine(row);
        std::memcpy(result.data() + static_cast<std::size_t>(row) * dstW * 4, scanline, dstW * 4);
    }

    return result;
}

QSize FloatingBuffer::getScaledSize(QSizeF scale) const
{
    return {static_cast<int>(std::round(sourceRect_.width() * scale.width())),
            static_cast<int>(std::round(sourceRect_.height() * scale.height()))};
}

bool FloatingBuffer::isPixelSelected(int col, int row) const
{
    if (selectionMask_.empty() || sourceRect_.isEmpty()) {
        return false;
    }
    int width = sourceRect_.width();
    if (col < 0 || col >= width || row < 0 || row >= sourceRect_.height()) {
        return false;
    }
    return selectionMask_[static_cast<std::size_t>(row) * width + col];
}

void FloatingBuffer::rasterizeSelectionMask(const QPainterPath& selPath, const QRect& bounds)
{
    int width = bounds.width();
    int height = bounds.height();
    int x1 = bounds.left();
    int y1 = bounds.top();

    selectionMask_.resize(static_cast<std::size_t>(width) * height, false);

    // Use selection type hint for optimized rasterization
    SelectionType selType = SelectionManager::instance().selectionType();
    QRectF pathBounds = selPath.boundingRect();

    if (selType == SelectionType::Rectangle) {
        // Rectangle: Direct bounds check (O(1) per pixel)
        int rectX1 = static_cast<int>(std::floor(pathBounds.left()));
        int rectY1 = static_cast<int>(std::floor(pathBounds.top()));
        int rectX2 = static_cast<int>(std::ceil(pathBounds.right()));
        int rectY2 = static_cast<int>(std::ceil(pathBounds.bottom()));

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                int px = x1 + col;
                int py = y1 + row;
                selectionMask_[static_cast<std::size_t>(row) * width + col] =
                    (px >= rectX1 && px < rectX2 && py >= rectY1 && py < rectY2);
            }
        }
    } else if (selType == SelectionType::Ellipse) {
        // Ellipse: Direct equation check (O(1) per pixel)
        double cx = pathBounds.center().x();
        double cy = pathBounds.center().y();
        double rx = pathBounds.width() / 2.0;
        double ry = pathBounds.height() / 2.0;

        // Avoid division by zero
        if (rx < 0.5 || ry < 0.5) {
            return;
        }

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                double px = x1 + col + 0.5;
                double py = y1 + row + 0.5;
                double dx = (px - cx) / rx;
                double dy = (py - cy) / ry;
                selectionMask_[static_cast<std::size_t>(row) * width + col] =
                    (dx * dx + dy * dy) <= 1.0;
            }
        }
    } else {
        // Unknown/freeform: Use QPainter rasterization
        QImage maskImage(width, height, QImage::Format_Grayscale8);
        maskImage.fill(0);

        QPainter painter(&maskImage);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.translate(-x1, -y1);
        painter.drawPath(selPath);
        painter.end();

        for (int row = 0; row < height; ++row) {
            const uchar* scanline = maskImage.constScanLine(row);
            for (int col = 0; col < width; ++col) {
                selectionMask_[static_cast<std::size_t>(row) * width + col] = (scanline[col] > 0);
            }
        }
    }
}

}  // namespace gimp
