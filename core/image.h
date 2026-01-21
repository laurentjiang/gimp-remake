/**
 * @file image.h
 * @brief Minimal image file interface for reading and writing.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include <opencv2/core.hpp>

#include <string>

namespace gimp {

class Image {
  public:
    virtual ~Image() = default;

    virtual const cv::Mat& mat() const = 0;
    virtual cv::Mat& mat() = 0;
    virtual const std::string& file_path() const = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int channels() const = 0;
    virtual int depth() const = 0;
    virtual bool empty() const = 0;

    virtual void to_grayscale() = 0;
    virtual void to_rgb() = 0;
    virtual void to_rgba() = 0;
};

}  // namespace gimp
