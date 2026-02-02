/**
 * @file blur_filter.cpp
 * @brief Implementation of BlurFilter.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#include "core/filters/blur_filter.h"

#include "core/layer.h"

#include <algorithm>
#include <cmath>

namespace gimp {

void BlurFilter::setRadius(float radius)
{
    radius_ = std::clamp(radius, 1.0F, 100.0F);
}

std::vector<float> BlurFilter::generateGaussianKernel(float radius)
{
    int size = static_cast<int>(std::ceil(radius)) * 2 + 1;
    std::vector<float> kernel(size);
    float sigma = radius / 3.0F;
    float sum = 0.0F;

    int center = size / 2;
    for (int i = 0; i < size; ++i) {
        float distance = static_cast<float>(i - center);
        kernel[i] = std::exp(-(distance * distance) / (2.0F * sigma * sigma));
        sum += kernel[i];
    }

    // Normalize kernel
    for (auto& value : kernel) {
        value /= sum;
    }

    return kernel;
}

void BlurFilter::applyHorizontalBlur(std::vector<std::uint8_t>& data,
                                     int width,
                                     int height,
                                     const std::vector<float>& kernel)
{
    std::vector<std::uint8_t> temp = data;
    int kernelRadius = kernel.size() / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float r = 0.0F, g = 0.0F, b = 0.0F, a = 0.0F;

            for (int i = 0; i < static_cast<int>(kernel.size()); ++i) {
                int px = x + (i - kernelRadius);
                px = std::clamp(px, 0, width - 1);

                int idx = (y * width + px) * 4;
                r += temp[idx] * kernel[i];
                g += temp[idx + 1] * kernel[i];
                b += temp[idx + 2] * kernel[i];
                a += temp[idx + 3] * kernel[i];
            }

            int idx = (y * width + x) * 4;
            data[idx] = static_cast<std::uint8_t>(std::clamp(r, 0.0F, 255.0F));
            data[idx + 1] = static_cast<std::uint8_t>(std::clamp(g, 0.0F, 255.0F));
            data[idx + 2] = static_cast<std::uint8_t>(std::clamp(b, 0.0F, 255.0F));
            data[idx + 3] = static_cast<std::uint8_t>(std::clamp(a, 0.0F, 255.0F));
        }
    }
}

void BlurFilter::applyVerticalBlur(std::vector<std::uint8_t>& data,
                                   int width,
                                   int height,
                                   const std::vector<float>& kernel)
{
    std::vector<std::uint8_t> temp = data;
    int kernelRadius = kernel.size() / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float r = 0.0F, g = 0.0F, b = 0.0F, a = 0.0F;

            for (int i = 0; i < static_cast<int>(kernel.size()); ++i) {
                int py = y + (i - kernelRadius);
                py = std::clamp(py, 0, height - 1);

                int idx = (py * width + x) * 4;
                r += temp[idx] * kernel[i];
                g += temp[idx + 1] * kernel[i];
                b += temp[idx + 2] * kernel[i];
                a += temp[idx + 3] * kernel[i];
            }

            int idx = (y * width + x) * 4;
            data[idx] = static_cast<std::uint8_t>(std::clamp(r, 0.0F, 255.0F));
            data[idx + 1] = static_cast<std::uint8_t>(std::clamp(g, 0.0F, 255.0F));
            data[idx + 2] = static_cast<std::uint8_t>(std::clamp(b, 0.0F, 255.0F));
            data[idx + 3] = static_cast<std::uint8_t>(std::clamp(a, 0.0F, 255.0F));
        }
    }
}

bool BlurFilter::apply(std::shared_ptr<Layer> layer)
{
    if (!layer || layer->data().empty()) {
        return false;
    }

    auto& data = layer->data();
    int width = layer->width();
    int height = layer->height();

    if (width <= 0 || height <= 0) {
        return false;
    }

    auto kernel = generateGaussianKernel(radius_);

    applyHorizontalBlur(data, width, height, kernel);
    applyVerticalBlur(data, width, height, kernel);

    return true;
}

bool BlurFilter::setParameter(const std::string& name, float value)
{
    if (name == "radius") {
        setRadius(value);
        return true;
    }
    return false;
}

bool BlurFilter::getParameter(const std::string& name, float& value) const
{
    if (name == "radius") {
        value = radius_;
        return true;
    }
    return false;
}

float BlurFilter::progress() const
{
    return Filter::progress();
}

bool BlurFilter::isRunning() const
{
    return Filter::isRunning();
}

}  // namespace gimp