/**
 * @file skia_compositor.cpp
 * @brief Implementation of SkiaCompositor.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "render/skia_compositor.h"

#include "core/layer.h"

#include <include/core/SkBitmap.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkImage.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkPaint.h>

namespace gimp {

namespace {

/// Converts layer blend mode to Skia blend mode.
SkBlendMode toSkBlendMode(BlendMode mode)
{
    switch (mode) {
        case BlendMode::Normal:
            return SkBlendMode::kSrcOver;
        case BlendMode::Multiply:
            return SkBlendMode::kMultiply;
        case BlendMode::Overlay:
            return SkBlendMode::kOverlay;
        case BlendMode::Screen:
            return SkBlendMode::kScreen;
        case BlendMode::Darken:
            return SkBlendMode::kDarken;
        case BlendMode::Lighten:
            return SkBlendMode::kLighten;
    }
    return SkBlendMode::kSrcOver;
}

/// Draws a single layer onto the canvas with its blend mode and opacity.
void drawLayerToCanvas(SkCanvas* canvas, const Layer& layer)
{
    const SkImageInfo info = SkImageInfo::Make(
        layer.width(), layer.height(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);

    SkBitmap bitmap;
    // Use const_cast because installPixels expects void* but we are reading
    if (!bitmap.installPixels(info,
                              const_cast<void*>(reinterpret_cast<const void*>(layer.data().data())),
                              info.minRowBytes())) {
        return;
    }

    SkPaint paint;
    paint.setAlphaf(layer.opacity());
    paint.setBlendMode(toSkBlendMode(layer.blendMode()));

    canvas->drawImage(bitmap.asImage(), 0, 0, SkSamplingOptions(), &paint);
}

}  // namespace

void SkiaCompositor::compose(SkCanvas* canvas, const LayerStack& layers)
{
    for (const auto& layer : layers) {
        if (!layer->visible())
            continue;
        drawLayerToCanvas(canvas, *layer);
    }
}

void SkiaCompositor::composeUpTo(SkCanvas* canvas,
                                 const LayerStack& layers,
                                 std::size_t stopBeforeIndex)
{
    std::size_t idx = 0;
    for (const auto& layer : layers) {
        if (idx >= stopBeforeIndex) {
            break;
        }
        if (layer->visible()) {
            drawLayerToCanvas(canvas, *layer);
        }
        ++idx;
    }
}

void SkiaCompositor::composeSingleLayer(SkCanvas* canvas, const Layer& layer)
{
    if (layer.visible()) {
        drawLayerToCanvas(canvas, layer);
    }
}

}  // namespace gimp
