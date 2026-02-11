/**
 * @file main.cpp
 * @brief Application entrypoint stub for the GIMP remake.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#include "io/io_manager.h"
#include "io/project_file.h"
#include "ui/main_window.h"

#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QSurfaceFormat>

#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>
#include <memory>

// Error handling system
#include "error_handling/error_codes.h"
#include "error_handling/error_handler.h"
#include "error_handling/error_result.h"
#include "error_handling/exceptions.h"

/**
 * @brief Set up error recovery handlers for the application
 */
void setupErrorRecoveryHandlers()
{
    // Register recovery handler for I/O errors
    gimp::error::ErrorHandler::GetInstance().RegisterRecoveryHandler(
        gimp::error::ErrorCategory::IO, [](const gimp::error::ErrorInfo& /*error*/) -> bool {
            std::cout << "Attempting I/O error recovery...\n";
            // In a real implementation, could try alternate file locations,
            // request elevated permissions, etc.
            return false;  // No automatic recovery implemented yet
        });

    // Register recovery handler for rendering errors
    gimp::error::ErrorHandler::GetInstance().RegisterRecoveryHandler(
        gimp::error::ErrorCategory::Rendering, [](const gimp::error::ErrorInfo& /*error*/) -> bool {
            std::cout << "Attempting rendering error recovery...\n";
            // In a real implementation, could try to reinitialize renderer,
            // fall back to software rendering, etc.
            return false;  // No automatic recovery implemented yet
        });

    // Register callback to show errors in UI
    gimp::error::ErrorHandler::GetInstance().RegisterCallback(
        [](const gimp::error::ErrorInfo& error) {
            if (error.GetSeverity() >= gimp::error::ErrorSeverity::Recoverable) {
                std::cerr << error.ToString() << '\n';
            }
        });
}

/**
 * @brief Run the main application with error handling
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int runApplication(int argc, char** argv)
{
    // Configure OpenGL surface format for Skia GPU rendering
    QSurfaceFormat format;
    format.setDepthBufferSize(0);    // Skia 2D doesn't need depth buffer, saves VRAM
    format.setStencilBufferSize(8);  // REQUIRED: Skia uses stencil for clipping/masking
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    const QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/gomp.png"));

    gimp::MainWindow window;

    // Log after MainWindow construction so QtForwardingSink is registered
    spdlog::info("Application starting up...");

    window.show();

    return QApplication::exec();
}

/*!
 * @brief Application entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit code.
 */
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv)
{
    // Initialize error handling system first
    gimp::error::ErrorHandler::GetInstance().Initialize();
    gimp::error::ErrorHandler::GetInstance().SetLogFilePath("gimp_remake_errors.log");

    // Set up recovery handlers
    setupErrorRecoveryHandlers();

    int exitCode = 0;

    try {
        // Run application with top-level exception handling
        exitCode = runApplication(argc, argv);

    } catch (const gimp::error::GimpException& e) {
        // Handle GIMP-specific exceptions
        std::cerr << "Caught GIMP exception: " << e.what() << '\n';

        if (e.GetSeverity() == gimp::error::ErrorSeverity::Fatal) {
            // Fatal error - must terminate
            gimp::error::ErrorHandler::GetInstance().HandleFatalError(e.GetErrorInfo());
        } else {
            // Recoverable error - report and exit
            gimp::error::ErrorHandler::GetInstance().ReportError(e.GetErrorInfo());
            exitCode = 1;
        }

    } catch (const std::exception& e) {
        // Handle standard exceptions
        std::cerr << "Caught standard exception: " << e.what() << '\n';

        const gimp::error::ErrorInfo error(gimp::error::ErrorCode::SystemUnknownError,
                                           std::string("Unhandled exception: ") + e.what());
        gimp::error::ErrorHandler::GetInstance().HandleFatalError(error);

    } catch (...) {
        // Catch-all for unknown exceptions
        std::cerr << "Caught unknown exception\n";

        const gimp::error::ErrorInfo error(gimp::error::ErrorCode::Unknown,
                                           "Unknown exception caught in main");
        gimp::error::ErrorHandler::GetInstance().HandleFatalError(error);
    }

    // Shutdown error handling system
    gimp::error::ErrorHandler::GetInstance().Shutdown();

    return exitCode;
}
