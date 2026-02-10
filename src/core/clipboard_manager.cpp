/**
 * @file clipboard_manager.cpp
 * @brief Implementation of ClipboardManager.
 * @author Aless Tosi
 * @date 2026-02-04
 */

#include "core/clipboard_manager.h"

#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/commands/paste_command.h"
#include "core/document.h"
#include "core/layer.h"
#include "core/selection_manager.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QPainterPath>
#include <QRectF>

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace gimp {

namespace {

bool selectionBounds(const QPainterPath& path,
                     int docWidth,
                     int docHeight,
                     int& outX,
                     int& outY,
                     int& outWidth,
                     int& outHeight)
{
    if (path.isEmpty()) {
        return false;
    }

    const QRectF boundsF = path.boundingRect();
    const QRect bounds = boundsF.toAlignedRect();

    const int left = std::max(0, bounds.left());
    const int top = std::max(0, bounds.top());
    const int right = std::min(docWidth - 1, bounds.right());
    const int bottom = std::min(docHeight - 1, bounds.bottom());

    if (right < left || bottom < top) {
        return false;
    }

    outX = left;
    outY = top;
    outWidth = right - left + 1;
    outHeight = bottom - top + 1;
    return true;
}

QImage toRgbaImage(const QImage& image)
{
    if (image.format() == QImage::Format_RGBA8888) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888);
}

}  // namespace

bool ClipboardManager::copySelection(const std::shared_ptr<Document>& document,
                                     const std::shared_ptr<Layer>& layer)
{
    if (!document || document->layers().count() == 0) {
        return false;
    }

    // Use provided layer or fall back to active layer
    auto sourceLayer = layer ? layer : document->activeLayer();
    if (!sourceLayer) {
        return false;
    }
    const int layerWidth = sourceLayer->width();
    const int layerHeight = sourceLayer->height();
    const auto& data = sourceLayer->data();

    const auto& selectionPath = SelectionManager::instance().selectionPath();

    // If no selection, copy entire layer (GIMP behavior)
    if (selectionPath.isEmpty()) {
        QImage image(layerWidth, layerHeight, QImage::Format_RGBA8888);
        std::memcpy(image.bits(), data.data(), data.size());
        setImageInternal(image);
        return true;
    }

    int regionX = 0;
    int regionY = 0;
    int regionWidth = 0;
    int regionHeight = 0;

    if (!selectionBounds(selectionPath,
                         document->width(),
                         document->height(),
                         regionX,
                         regionY,
                         regionWidth,
                         regionHeight)) {
        return false;
    }

    QImage image(regionWidth, regionHeight, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);

    for (int y = 0; y < regionHeight; ++y) {
        const int srcY = regionY + y;
        for (int x = 0; x < regionWidth; ++x) {
            const int srcX = regionX + x;
            if (!selectionPath.contains(QPointF(srcX + 0.5, srcY + 0.5))) {
                continue;
            }

            const std::size_t srcIndex =
                (static_cast<std::size_t>(srcY) * static_cast<std::size_t>(layerWidth) +
                 static_cast<std::size_t>(srcX)) *
                4U;
            const std::size_t dstIndex =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(regionWidth) +
                 static_cast<std::size_t>(x)) *
                4U;

            std::uint8_t* dest = image.bits();
            dest[dstIndex + 0] = data[srcIndex + 0];
            dest[dstIndex + 1] = data[srcIndex + 1];
            dest[dstIndex + 2] = data[srcIndex + 2];
            dest[dstIndex + 3] = data[srcIndex + 3];
        }
    }

    setImageInternal(image);
    return true;
}

bool ClipboardManager::cutSelection(const std::shared_ptr<Document>& document,
                                    const std::shared_ptr<Layer>& layer,
                                    CommandBus* commandBus)
{
    if (!document || document->layers().count() == 0) {
        return false;
    }

    // Cut requires a selection (unlike Copy which copies entire layer)
    const auto& selectionPath = SelectionManager::instance().selectionPath();
    if (selectionPath.isEmpty()) {
        return false;
    }

    if (!copySelection(document, layer)) {
        return false;
    }

    // Use provided layer or fall back to active layer
    auto targetLayer = layer ? layer : document->activeLayer();
    if (!targetLayer) {
        return false;
    }

    int regionX = 0;
    int regionY = 0;
    int regionWidth = 0;
    int regionHeight = 0;

    if (!selectionBounds(selectionPath,
                         document->width(),
                         document->height(),
                         regionX,
                         regionY,
                         regionWidth,
                         regionHeight)) {
        return false;
    }

    auto cutCommand =
        std::make_shared<DrawCommand>(targetLayer, regionX, regionY, regionWidth, regionHeight);
    cutCommand->captureBeforeState();

    auto& data = targetLayer->data();
    const int layerWidth = targetLayer->width();

    for (int y = 0; y < regionHeight; ++y) {
        const int srcY = regionY + y;
        for (int x = 0; x < regionWidth; ++x) {
            const int srcX = regionX + x;
            if (!selectionPath.contains(QPointF(srcX + 0.5, srcY + 0.5))) {
                continue;
            }

            const std::size_t dstIndex =
                (static_cast<std::size_t>(srcY) * static_cast<std::size_t>(layerWidth) +
                 static_cast<std::size_t>(srcX)) *
                4U;
            data[dstIndex + 0] = 0;
            data[dstIndex + 1] = 0;
            data[dstIndex + 2] = 0;
            data[dstIndex + 3] = 0;
        }
    }

    cutCommand->captureAfterState();

    if (commandBus) {
        commandBus->dispatch(cutCommand);
    }

    return true;
}

bool ClipboardManager::pasteToDocument(const std::shared_ptr<Document>& document,
                                       CommandBus* commandBus,
                                       const QPoint& canvasPos,
                                       bool useCursor)
{
    if (!document || document->layers().count() == 0) {
        return false;
    }

    if (image_.isNull()) {
        updateFromSystemClipboard();
    }

    if (image_.isNull()) {
        return false;
    }

    QImage sourceImage = toRgbaImage(image_);
    const int imageWidth = sourceImage.width();
    const int imageHeight = sourceImage.height();

    QPoint anchor = useCursor ? canvasPos : QPoint(document->width() / 2, document->height() / 2);
    QPoint topLeft = anchor - QPoint(imageWidth / 2, imageHeight / 2);

    int destX = topLeft.x();
    int destY = topLeft.y();
    int srcX = 0;
    int srcY = 0;

    if (destX < 0) {
        srcX = -destX;
        destX = 0;
    }
    if (destY < 0) {
        srcY = -destY;
        destY = 0;
    }

    const int availableWidth = document->width() - destX;
    const int availableHeight = document->height() - destY;
    const int pasteWidth = std::min(imageWidth - srcX, availableWidth);
    const int pasteHeight = std::min(imageHeight - srcY, availableHeight);

    if (pasteWidth <= 0 || pasteHeight <= 0) {
        return false;
    }

    QImage clipped = sourceImage.copy(srcX, srcY, pasteWidth, pasteHeight);

    auto pasteCommand = std::make_shared<PasteCommand>(document, destX, destY, clipped);

    if (commandBus) {
        commandBus->dispatch(pasteCommand);
    } else {
        pasteCommand->apply();
    }

    SelectionManager::instance().clear();
    QPainterPath selectionRect;
    selectionRect.addRect(QRectF(destX, destY, pasteWidth, pasteHeight));
    SelectionManager::instance().applySelection(selectionRect, SelectionMode::Replace);

    return true;
}

bool ClipboardManager::updateFromSystemClipboard()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        return false;
    }

    const QImage image = clipboard->image();
    if (image.isNull()) {
        return false;
    }

    setImageInternal(image);
    return true;
}

void ClipboardManager::setImageInternal(const QImage& image)
{
    image_ = toRgbaImage(image);

    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        clipboard->setImage(image_);
    }
}

}  // namespace gimp
