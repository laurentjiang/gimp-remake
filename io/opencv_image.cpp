/**
 * @file opencv_image.cpp
 * @brief Implementation of gimp::OpenCVImage class
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/opencv_image.h"

#include <opencv2/imgproc.hpp>

namespace gimp {

void OpenCVImage::to_grayscale()
{
    if (m_mat.channels() == 3) {
        cv::cvtColor(m_mat, m_mat, cv::COLOR_RGB2GRAY);
    } else if (m_mat.channels() == 4) {
        cv::cvtColor(m_mat, m_mat, cv::COLOR_RGBA2GRAY);
    }
    // 1 channel (already grayscale): no-op
}

void OpenCVImage::to_rgb()
{
    if (m_mat.channels() == 1) {
        cv::cvtColor(m_mat, m_mat, cv::COLOR_GRAY2RGB);
    } else if (m_mat.channels() == 4) {
        cv::cvtColor(m_mat, m_mat, cv::COLOR_RGBA2RGB);
    }
    // 3 channels (already RGB): no-op
}

void OpenCVImage::to_rgba()
{
    if (m_mat.channels() == 1) {
        cv::cvtColor(m_mat, m_mat, cv::COLOR_GRAY2RGBA);
    } else if (m_mat.channels() == 3) {
        cv::cvtColor(m_mat, m_mat, cv::COLOR_RGB2RGBA);
    }
    // 4 channels (already RGBA): no-op
}

}  // namespace gimp
