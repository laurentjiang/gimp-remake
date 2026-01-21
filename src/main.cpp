/**
 * @file main.cpp
 * @brief Application entrypoint stub for the GIMP remake.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#include "ui/main_window.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);

    gimp::MainWindow window;

    window.show();

    return QApplication::exec();
}
