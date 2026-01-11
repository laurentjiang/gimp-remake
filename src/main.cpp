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

#include "io/io_manager.h"

int main(int argc, char* argv[])
{
    gimp::IOManager ioManager;
    std::shared_ptr<gimp::ImageFile> image = std::make_shared<gimp::ImageFile>(ioManager.readImage("starry_night.jpg"));
    cv::Mat imgMat = image->mat().clone();
    std::string imgPath = image->file_path();

    if (!image || image->empty()) {
        std::cerr << "Failed to open starry_night.jpg" << std::endl;
    } else {
        // Write grayscale
        cv::Mat grayMat = imgMat.clone();
        ioManager.toGrayscale(grayMat);
        ioManager.writeImage(grayMat, "starry_night_gray.jpg");

        // Write RGB
        cv::Mat rgbMat = imgMat.clone();
        ioManager.toRgb(rgbMat);
        ioManager.writeImage(rgbMat, "starry_night_rgb.jpg");

        // Write RGBA
        cv::Mat rgbaMat = imgMat.clone();
        ioManager.toRgba(rgbaMat);
        ioManager.writeImage(rgbaMat, "starry_night_rgba.jpg");
    }

    const QApplication app(argc, argv);

    gimp::MainWindow window;
    window.show();

    return QApplication::exec();
}
