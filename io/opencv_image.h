/**
 * @file opencv_image.h
 * @brief gimp::OpenCVImage class definition, subclass of gimp::Image
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "../core/image.h"

namespace gimp {

// Example concrete Image implementation for OpenCV
class OpenCVImage : public Image {
  public:
    OpenCVImage(const cv::Mat& m, const std::string& path) : m_mat(m), m_path(path) {}
    ~OpenCVImage() override = default;

    const cv::Mat& mat() const override { return m_mat; }
    cv::Mat& mat() override { return m_mat; }
    const std::string& file_path() const override { return m_path; }

    int width() const override { return m_mat.cols; }
    int height() const override { return m_mat.rows; }
    int channels() const override { return m_mat.channels(); }
    int depth() const override { return m_mat.depth(); }
    bool empty() const override { return m_mat.empty(); }
    
    void to_grayscale() override;
    void to_rgb() override;
    void to_rgba() override;

  private:
    cv::Mat m_mat;
    std::string m_path;
};

} // namespace gimp
