/**
 * @file main.cpp
 * @brief Application entrypoint stub for the GIMP remake.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#include <QApplication>
#include "ui/main_window.h"

#include <memory>
#include <iostream>
#include "io/opencv_image.h"
#include "io/io_manager.h"

int main(int argc, char* argv[])
{
    gimp::IOManager io_manager;
    std::shared_ptr<gimp::Image> image = io_manager.read_image("starry_night.jpg");
    if (!image || image->empty()) {
        std::cerr << "Failed to open starry_night.jpg" << std::endl;
    } else {
        // Write grayscale
        auto img_gray = std::make_shared<gimp::OpenCVImage>(image->mat().clone(), image->file_path());
        img_gray->to_grayscale();
        io_manager.write_image(*img_gray, "starry_night_gray.jpg");

        // Write RGB
        auto img_rgb = std::make_shared<gimp::OpenCVImage>(image->mat().clone(), image->file_path());
        img_rgb->to_rgb();
        io_manager.write_image(*img_rgb, "starry_night_rgb.jpg");

        // Write RGBA
        auto img_rgba = std::make_shared<gimp::OpenCVImage>(image->mat().clone(), image->file_path());
        img_rgba->to_rgba();
        io_manager.write_image(*img_rgba, "starry_night_rgba.jpg");
    }

    const QApplication app(argc, argv);

    gimp::MainWindow window;
    window.show();

    return QApplication::exec();
}
