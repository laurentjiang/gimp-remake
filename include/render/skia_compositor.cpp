/**
 * @file skia_compositor.cpp
 * @brief Implementation of SkiaCompositor.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "skia_compositor.h"
#include <include/core/SkBitmap.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkImage.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkPaint.h>
#include "core/layer.h"

namespace gimp {

void SkiaCompositor::compose(SkCanvas* canvas, const LayerStack& layers)
{
    for (const auto& layer : layers) {
        if (!layer->visible())
            continue;

        const SkImageInfo info = SkImageInfo::Make(
            layer->width(), layer->height(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);

        SkBitmap bitmap;
        // Use const_cast because installPixels expects void* but we are reading
        if (!bitmap.installPixels(
                info,
                const_cast<void*>(reinterpret_cast<const void*>(layer->data().data())),
                info.minRowBytes())) {
            continue;
        }

        SkPaint paint;
        paint.setAlphaf(layer->opacity());

        SkBlendMode mode = SkBlendMode::kSrcOver;
        switch (layer->blend_mode()) {
            case BlendMode::Normal:
                mode = SkBlendMode::kSrcOver;
                break;
            case BlendMode::Multiply:
                mode = SkBlendMode::kMultiply;
                break;
            case BlendMode::Overlay:
                mode = SkBlendMode::kOverlay;
                break;
            case BlendMode::Screen:
                mode = SkBlendMode::kScreen;
                break;
            case BlendMode::Darken:
                mode = SkBlendMode::kDarken;
                break;
            case BlendMode::Lighten:
                mode = SkBlendMode::kLighten;
                break;
        }
        paint.setBlendMode(mode);

        canvas->drawImage(bitmap.asImage(), 0, 0, SkSamplingOptions(), &paint);
    }
}

}  // namespace gimp
