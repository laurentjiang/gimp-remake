/**
 * @file sharpen_filter.cpp
 * @brief Implementation of SharpenFilter.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#include "core/filters/sharpen_filter.h"

#include "core/filters/blur_filter.h"
#include "core/layer.h"

#include <algorithm>

namespace gimp {

void SharpenFilter::setAmount(float amount)
{
    amount_ = std::clamp(amount, 0.0F, 2.0F);
}

void SharpenFilter::setRadius(float radius)
{
    radius_ = std::clamp(radius, 1.0F, 50.0F);
}

std::vector<std::uint8_t> SharpenFilter::createBlurredCopy(const std::vector<std::uint8_t>& data,
                                                           int width,
                                                           int height)
{
    // Create a temporary layer with the data
    auto tempLayer = std::make_shared<Layer>(width, height);
    tempLayer->data() = data;

    // Apply blur using BlurFilter
    BlurFilter blurFilter;
    blurFilter.setRadius(radius_);
    blurFilter.apply(tempLayer);

    return tempLayer->data();
}

bool SharpenFilter::apply(std::shared_ptr<Layer> layer)
{
    if (!layer || layer->data().empty()) {
        return false;
    }

    auto& data = layer->data();
    int width = layer->width();
    int height = layer->height();

    if (width <= 0 || height <= 0 || amount_ < 0.001F) {
        return false;
    }

    // Create blurred version
    auto blurred = createBlurredCopy(data, width, height);

    // Apply unsharp masking: output = original + amount * (original - blurred)
    for (std::size_t i = 0; i < data.size(); ++i) {
        float orig = static_cast<float>(data[i]);
        float blur = static_cast<float>(blurred[i]);
        float sharpened = orig + (orig - blur) * amount_;

        data[i] = static_cast<std::uint8_t>(std::clamp(sharpened, 0.0F, 255.0F));
    }

    return true;
}

bool SharpenFilter::setParameter(const std::string& name, float value)
{
    if (name == "amount") {
        setAmount(value);
        return true;
    } else if (name == "radius") {
        setRadius(value);
        return true;
    }
    return false;
}

bool SharpenFilter::getParameter(const std::string& name, float& value) const
{
    if (name == "amount") {
        value = amount_;
        return true;
    } else if (name == "radius") {
        value = radius_;
        return true;
    }
    return false;
}

float SharpenFilter::progress() const
{
    return Filter::progress();
}

bool SharpenFilter::isRunning() const
{
    return Filter::isRunning();
}

}  // namespace gimp