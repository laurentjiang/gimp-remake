/**
 * @file image_file.h
 * @brief gimp::ImageFile class definition.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include <string>
#include <opencv2/core.hpp>

namespace gimp {

    class ImageFile {
        public:
            ImageFile(const cv::Mat& m, const std::string& path) : m_mat(m), m_path(path) {}
            ~ImageFile() = default;

            const cv::Mat& mat() const { return m_mat; }
            cv::Mat& mat() { return m_mat; }
            const std::string& file_path() const { return m_path; }

            int width() const { return m_mat.cols; }
            int height() const { return m_mat.rows; }
            int channels() const { return m_mat.channels(); }
            int depth() const { return m_mat.depth(); }
            bool empty() const { return m_mat.empty(); }

        private:
            cv::Mat m_mat;
            std::string m_path;
    }; // class ImageFile

} // namespace gimp
