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
#include "io/project_file.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);

    gimp::MainWindow window;
    
    window.show();

    return QApplication::exec();
}
