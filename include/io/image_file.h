/**
 * @file image_file.h
 * @brief gimp::ImageFile class definition.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include <opencv2/core.hpp>

#include <string>

namespace gimp {

/*!
 * @class ImageFile
 * @brief Wrapper around cv::Mat that also stores the source file path.
 */
class ImageFile {
  public:
    /*!
     * @brief Constructs an ImageFile from image data and path.
     * @param m The OpenCV matrix containing image data.
     * @param path The source file path.
     */
    ImageFile(const cv::Mat& m, const std::string& path) : m_mat(m), m_path(path) {}

    /*! @brief Copy constructor. */
    ImageFile(const ImageFile& other) : m_mat(other.m_mat), m_path(other.m_path) {}

    ~ImageFile() = default;

    /*! @brief Returns a const reference to the image matrix. */
    const cv::Mat& mat() const { return m_mat; }

    /*! @brief Returns a mutable reference to the image matrix. */
    cv::Mat& mat() { return m_mat; }

    /*! @brief Returns the source file path. */
    const std::string& file_path() const { return m_path; }

    /*! @brief Returns the image width in pixels. */
    int width() const { return m_mat.cols; }

    /*! @brief Returns the image height in pixels. */
    int height() const { return m_mat.rows; }

    /*! @brief Returns the number of color channels. */
    int channels() const { return m_mat.channels(); }

    /*! @brief Returns the bit depth of the image. */
    int depth() const { return m_mat.depth(); }

    /*! @brief Returns true if the image contains no data. */
    bool empty() const { return m_mat.empty(); }

  private:
    cv::Mat m_mat;       ///< Image pixel data.
    std::string m_path;  ///< Source file path.
};  // class ImageFile

}  // namespace gimp
