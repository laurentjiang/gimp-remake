/**
 * @file main.cpp
 * @brief Application entrypoint stub for the GIMP remake.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#include <QApplication>
#include "ui/main_window.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    gimp::MainWindow window;
    window.show();

    return app.exec();
}
