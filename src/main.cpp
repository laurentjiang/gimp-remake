/**
 * @file main.cpp
 * @brief Application entrypoint stub for the GIMP remake.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#include <QApplication>
#include <QMessageBox>
#include "ui/main_window.h"

#include <memory>
#include <iostream>
#include <exception>

#include "io/io_manager.h"
#include "io/project_file.h"

// Error handling system
#include "error_handling/error_codes.h"
#include "error_handling/error_result.h"
#include "error_handling/error_handler.h"
#include "error_handling/exceptions.h"

/**
 * @brief Set up error recovery handlers for the application
 */
void SetupErrorRecoveryHandlers() {
    using namespace gimp::error;
    
    // Register recovery handler for I/O errors
    ErrorHandler::GetInstance().RegisterRecoveryHandler(
        ErrorCategory::IO,
        [](const ErrorInfo& error) -> bool {
            std::cout << "Attempting I/O error recovery..." << std::endl;
            // In a real implementation, could try alternate file locations,
            // request elevated permissions, etc.
            return false;  // No automatic recovery implemented yet
        }
    );
    
    // Register recovery handler for rendering errors
    ErrorHandler::GetInstance().RegisterRecoveryHandler(
        ErrorCategory::Rendering,
        [](const ErrorInfo& error) -> bool {
            std::cout << "Attempting rendering error recovery..." << std::endl;
            // In a real implementation, could try to reinitialize renderer,
            // fall back to software rendering, etc.
            return false;  // No automatic recovery implemented yet
        }
    );
    
    // Register callback to show errors in UI
    ErrorHandler::GetInstance().RegisterCallback(
        [](const ErrorInfo& error) {
            if (error.GetSeverity() >= ErrorSeverity::Recoverable) {
                std::cerr << error.ToString() << std::endl;
            }
        }
    );
}

/**
 * @brief Run the main application with error handling
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int RunApplication(int argc, char* argv[]) {
    const QApplication app(argc, argv);

    gimp::MainWindow window;
    window.show();

    return QApplication::exec();
}

int main(int argc, char* argv[])
{
    using namespace gimp::error;
    
    // Initialize error handling system first
    ErrorHandler::GetInstance().Initialize();
    ErrorHandler::GetInstance().SetLogFilePath("gimp_remake_errors.log");
    
    // Set up recovery handlers
    SetupErrorRecoveryHandlers();
    
    int exit_code = 0;
    
    try {
        // Run application with top-level exception handling
        exit_code = RunApplication(argc, argv);
        
    } catch (const GimpException& e) {
        // Handle GIMP-specific exceptions
        std::cerr << "Caught GIMP exception: " << e.what() << std::endl;
        
        if (e.GetSeverity() == ErrorSeverity::Fatal) {
            // Fatal error - must terminate
            ErrorHandler::GetInstance().HandleFatalError(e.GetErrorInfo());
        } else {
            // Recoverable error - report and exit
            ErrorHandler::GetInstance().ReportError(e.GetErrorInfo());
            exit_code = 1;
        }
        
    } catch (const std::exception& e) {
        // Handle standard exceptions
        std::cerr << "Caught standard exception: " << e.what() << std::endl;
        
        ErrorInfo error(ErrorCode::SystemUnknownError, 
                       std::string("Unhandled exception: ") + e.what());
        ErrorHandler::GetInstance().HandleFatalError(error);
        
    } catch (...) {
        // Catch-all for unknown exceptions
        std::cerr << "Caught unknown exception" << std::endl;
        
        ErrorInfo error(ErrorCode::Unknown, "Unknown exception caught in main");
        ErrorHandler::GetInstance().HandleFatalError(error);
    }
    
    // Shutdown error handling system
    ErrorHandler::GetInstance().Shutdown();
    
    return exit_code;
}
