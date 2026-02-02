/**
 * @file brush_strategy.cpp
 * @brief Implementation of brush strategies.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/brush_strategy.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gimp {

namespace {

/**
 * @brief Extracts color components from RGBA packed format.
 * @param rgba Packed color (0xRRGGBBAA).
 * @param r Output red component.
 * @param g Output green component.
 * @param b Output blue component.
 * @param a Output alpha component.
 */
void unpackRGBA(std::uint32_t rgba,
                std::uint8_t& r,
                std::uint8_t& g,
                std::uint8_t& b,
                std::uint8_t& a)
{
    r = static_cast<std::uint8_t>((rgba >> 24) & 0xFF);
    g = static_cast<std::uint8_t>((rgba >> 16) & 0xFF);
    b = static_cast<std::uint8_t>((rgba >> 8) & 0xFF);
    a = static_cast<std::uint8_t>(rgba & 0xFF);
}

/**
 * @brief Blends a source color over a destination with alpha.
 * @param dst Pointer to destination pixel (RGBA).
 * @param sr Source red.
 * @param sg Source green.
 * @param sb Source blue.
 * @param sa Source alpha.
 */
void blendPixel(std::uint8_t* dst,
                std::uint8_t sr,
                std::uint8_t sg,
                std::uint8_t sb,
                std::uint8_t sa)
{
    if (sa == 0) {
        return;
    }

    std::uint8_t dr = dst[0];
    std::uint8_t dg = dst[1];
    std::uint8_t db = dst[2];
    std::uint8_t da = dst[3];

    if (sa == 255 || da == 0) {
        dst[0] = sr;
        dst[1] = sg;
        dst[2] = sb;
        dst[3] = sa;
        return;
    }

    // Porter-Duff "over" compositing
    float srcA = static_cast<float>(sa) / 255.0F;
    float dstA = static_cast<float>(da) / 255.0F;
    float outA = srcA + dstA * (1.0F - srcA);

    if (outA > 0.0F) {
        dst[0] = static_cast<std::uint8_t>(
            (static_cast<float>(sr) * srcA + static_cast<float>(dr) * dstA * (1.0F - srcA)) / outA);
        dst[1] = static_cast<std::uint8_t>(
            (static_cast<float>(sg) * srcA + static_cast<float>(dg) * dstA * (1.0F - srcA)) / outA);
        dst[2] = static_cast<std::uint8_t>(
            (static_cast<float>(sb) * srcA + static_cast<float>(db) * dstA * (1.0F - srcA)) / outA);
        dst[3] = static_cast<std::uint8_t>(outA * 255.0F);
    }
}

}  // namespace

void SolidBrush::renderDab(std::uint8_t* target,
                           int targetWidth,
                           int targetHeight,
                           int x,
                           int y,
                           int size,
                           std::uint32_t color,
                           float pressure)
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
    unpackRGBA(color, r, g, b, a);

    // Apply pressure to alpha
    a = static_cast<std::uint8_t>(static_cast<float>(a) * pressure);

    int radius = size / 2;
    int radiusSq = radius * radius;

    int minX = std::max(0, x - radius);
    int maxX = std::min(targetWidth - 1, x + radius);
    int minY = std::max(0, y - radius);
    int maxY = std::min(targetHeight - 1, y + radius);

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            int dx = px - x;
            int dy = py - y;
            if (dx * dx + dy * dy <= radiusSq) {
                std::uint8_t* pixel = target + (py * targetWidth + px) * 4;
                blendPixel(pixel, r, g, b, a);
            }
        }
    }
}

void SoftBrush::renderDab(std::uint8_t* target,
                          int targetWidth,
                          int targetHeight,
                          int x,
                          int y,
                          int size,
                          std::uint32_t color,
                          float pressure)
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
    unpackRGBA(color, r, g, b, a);

    float radius = static_cast<float>(size) / 2.0F;
    if (radius < 0.5F) {
        radius = 0.5F;
    }

    int minX = std::max(0, x - static_cast<int>(radius) - 1);
    int maxX = std::min(targetWidth - 1, x + static_cast<int>(radius) + 1);
    int minY = std::max(0, y - static_cast<int>(radius) - 1);
    int maxY = std::min(targetHeight - 1, y + static_cast<int>(radius) + 1);

    // Gaussian sigma based on hardness: lower hardness = larger sigma = softer
    // At hardness=1.0, we want a nearly solid circle
    // At hardness=0.0, we want maximum blur
    float sigma = radius * (1.0F - hardness_ * 0.8F);
    if (sigma < 0.1F) {
        sigma = 0.1F;
    }
    float twoSigmaSq = 2.0F * sigma * sigma;

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            float dx = static_cast<float>(px - x);
            float dy = static_cast<float>(py - y);
            float distSq = dx * dx + dy * dy;
            float dist = std::sqrt(distSq);

            if (dist > radius) {
                continue;
            }

            // Gaussian falloff combined with hardness
            float falloff = 1.0F;
            if (hardness_ < 1.0F) {
                // Gaussian falloff from center
                falloff = std::exp(-distSq / twoSigmaSq);
                // Mix with hard edge based on hardness
                float hardEdge = (dist <= radius) ? 1.0F : 0.0F;
                falloff = hardness_ * hardEdge + (1.0F - hardness_) * falloff;
            }

            std::uint8_t finalAlpha =
                static_cast<std::uint8_t>(static_cast<float>(a) * pressure * falloff);

            std::uint8_t* pixel = target + (py * targetWidth + px) * 4;
            blendPixel(pixel, r, g, b, finalAlpha);
        }
    }
}

void StampBrush::setStamp(std::vector<std::uint8_t> data, int width, int height)
{
    stampData_ = std::move(data);
    stampWidth_ = width;
    stampHeight_ = height;
}

void StampBrush::renderDab(std::uint8_t* target,
                           int targetWidth,
                           int targetHeight,
                           int x,
                           int y,
                           int size,
                           std::uint32_t color,
                           float pressure)
{
    if (stampData_.empty() || stampWidth_ <= 0 || stampHeight_ <= 0) {
        return;
    }

    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
    unpackRGBA(color, r, g, b, a);

    // Scale factor from stamp size to requested size
    float scale =
        static_cast<float>(size) / static_cast<float>(std::max(stampWidth_, stampHeight_));
    int halfSize = size / 2;

    int minX = std::max(0, x - halfSize);
    int maxX = std::min(targetWidth - 1, x + halfSize);
    int minY = std::max(0, y - halfSize);
    int maxY = std::min(targetHeight - 1, y + halfSize);

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            // Map target pixel back to stamp coordinates
            float stampX = static_cast<float>(px - x + halfSize) / scale;
            float stampY = static_cast<float>(py - y + halfSize) / scale;

            int sx = static_cast<int>(stampX);
            int sy = static_cast<int>(stampY);

            if (sx >= 0 && sx < stampWidth_ && sy >= 0 && sy < stampHeight_) {
                std::uint8_t stampAlpha = stampData_[sy * stampWidth_ + sx];
                std::uint8_t finalAlpha = static_cast<std::uint8_t>(
                    static_cast<float>(a) * static_cast<float>(stampAlpha) / 255.0F * pressure);

                std::uint8_t* pixel = target + (py * targetWidth + px) * 4;
                blendPixel(pixel, r, g, b, finalAlpha);
            }
        }
    }
}

std::unique_ptr<BrushStrategy> createBrushStrategy(const char* typeName)
{
    if (std::strcmp(typeName, "solid") == 0) {
        return std::make_unique<SolidBrush>();
    }
    if (std::strcmp(typeName, "soft") == 0) {
        return std::make_unique<SoftBrush>();
    }
    if (std::strcmp(typeName, "stamp") == 0) {
        return std::make_unique<StampBrush>();
    }
    return nullptr;
}

}  // namespace gimp
