/**
 * @file error_handler.cpp
 * @brief Implementation of the application-wide error handler
 * @author Aless Tosi
 * @date 2026-01-14
 */

#include "error_handling/error_handler.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <QMessageBox>
#include <QApplication>

namespace gimp {
namespace error {

// Static context stack for error context tracking
static thread_local std::vector<std::string> g_context_stack;

// ErrorHandler Implementation

ErrorHandler& ErrorHandler::GetInstance() {
    static ErrorHandler instance;
    return instance;
}

ErrorHandler::ErrorHandler()
    : initialized_(false)
    , show_error_dialogs_(true)
    , log_file_path_("gimp_remake_errors.log") {
}

ErrorHandler::~ErrorHandler() {
    if (initialized_) {
        Shutdown();
    }
}

void ErrorHandler::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return;
    }
    
    // Set up crash handlers (platform-specific implementation would go here)
    // For now, we just mark as initialized
    initialized_ = true;
    
    // Log initialization
    std::cout << "Error handler initialized. Log file: " << log_file_path_ << std::endl;
}

void ErrorHandler::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }
    
    // Flush any pending logs
    FlushLogs();
    
    initialized_ = false;
    
    std::cout << "Error handler shut down." << std::endl;
}

void ErrorHandler::ReportError(const ErrorInfo& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add to history
    error_history_.push_back(error);
    
    // Log the error
    LogError(error);
    
    // Invoke callbacks for this error category
    for (const auto& [category, callback] : callbacks_) {
        if (category == ErrorCategory::Unknown || category == error.GetCategory()) {
            callback(error);
        }
    }
    
    // Handle based on severity
    switch (error.GetSeverity()) {
        case ErrorSeverity::Info:
        case ErrorSeverity::Warning:
            // Just log, don't show dialog
            std::cout << error.ToString() << std::endl;
            break;
            
        case ErrorSeverity::Recoverable:
            // Try to recover
            std::cerr << error.ToString() << std::endl;
            if (!AttemptRecovery(error)) {
                // Recovery failed, show error dialog if enabled
                if (show_error_dialogs_) {
                    ShowErrorDialog(error);
                }
            }
            break;
            
        case ErrorSeverity::Fatal:
            // Fatal errors are handled separately
            std::cerr << "FATAL: " << error.ToString() << std::endl;
            // Note: HandleFatalError should be called directly for fatal errors
            break;
    }
}

void ErrorHandler::ReportError(ErrorCode code, const std::string& context) {
    ReportError(ErrorInfo(code, context));
}

[[noreturn]] void ErrorHandler::HandleFatalError(const ErrorInfo& error) {
    std::cerr << "========================================" << std::endl;
    std::cerr << "FATAL ERROR - APPLICATION WILL TERMINATE" << std::endl;
    std::cerr << error.ToString() << std::endl;
    std::cerr << "========================================" << std::endl;
    
    // Log the fatal error
    LogError(error);
    
    // Attempt emergency save
    PerformEmergencySave();
    
    // Flush logs
    FlushLogs();
    
    // Show error dialog (even if dialogs are disabled for non-fatal errors)
    if (QApplication::instance()) {
        QMessageBox::critical(
            nullptr,
            "Fatal Error",
            QString::fromStdString(
                "A fatal error has occurred and the application must close.\n\n" +
                error.ToString() + "\n\n" +
                "An error log has been saved to: " + log_file_path_
            )
        );
    }
    
    // Terminate
    std::exit(EXIT_FAILURE);
}

bool ErrorHandler::AttemptRecovery(const ErrorInfo& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Try recovery handlers for this category
    for (const auto& [category, recovery_handler] : recovery_handlers_) {
        if (category == error.GetCategory()) {
            if (recovery_handler(error)) {
                std::cout << "Successfully recovered from error: " 
                         << error.GetMessage() << std::endl;
                return true;
            }
        }
    }
    
    return false;
}

void ErrorHandler::RegisterCallback(ErrorCallback callback, ErrorCategory category) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.emplace_back(category, std::move(callback));
}

void ErrorHandler::RegisterRecoveryHandler(ErrorCategory category, RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    recovery_handlers_.emplace_back(category, std::move(callback));
}

ErrorInfo ErrorHandler::GetLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (error_history_.empty()) {
        return ErrorInfo(); // Success
    }
    return error_history_.back();
}

std::vector<ErrorInfo> ErrorHandler::GetErrorHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error_history_;
}

void ErrorHandler::ClearHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    error_history_.clear();
}

void ErrorHandler::SetShowErrorDialogs(bool show) {
    std::lock_guard<std::mutex> lock(mutex_);
    show_error_dialogs_ = show;
}

bool ErrorHandler::GetShowErrorDialogs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return show_error_dialogs_;
}

void ErrorHandler::SetLogFilePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_file_path_ = path;
}

std::string ErrorHandler::GetLogFilePath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_file_path_;
}

void ErrorHandler::LogError(const ErrorInfo& error) {
    // Get current timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << " | " << error.ToString() << std::endl;
    
    // Write to log file
    std::ofstream log_file(log_file_path_, std::ios::app);
    if (log_file.is_open()) {
        log_file << oss.str();
        log_file.flush();
    }
}

void ErrorHandler::ShowErrorDialog(const ErrorInfo& error) {
    if (!QApplication::instance()) {
        return; // Can't show dialog without QApplication
    }
    
    QString title;
    QMessageBox::Icon icon;
    
    switch (error.GetSeverity()) {
        case ErrorSeverity::Warning:
            title = "Warning";
            icon = QMessageBox::Warning;
            break;
        case ErrorSeverity::Recoverable:
            title = "Error";
            icon = QMessageBox::Critical;
            break;
        case ErrorSeverity::Fatal:
            title = "Fatal Error";
            icon = QMessageBox::Critical;
            break;
        default:
            title = "Information";
            icon = QMessageBox::Information;
            break;
    }
    
    QMessageBox::information(
        nullptr,
        title,
        QString::fromStdString(error.ToString()),
        QMessageBox::Ok
    );
}

void ErrorHandler::PerformEmergencySave() {
    std::cout << "Attempting emergency save..." << std::endl;
    
    // TODO: Implement emergency save logic
    // This would:
    // 1. Save all open documents to backup locations
    // 2. Save application state
    // 3. Save undo history if possible
    
    std::cout << "Emergency save completed (not implemented yet)." << std::endl;
}

void ErrorHandler::FlushLogs() {
    // Logs are flushed after each write, so nothing to do here
    // This method exists for future expansion
}

// ErrorContext Implementation

ErrorContext::ErrorContext(std::string context_name)
    : context_name_(std::move(context_name)) {
    GetContextStack().push_back(context_name_);
}

ErrorContext::~ErrorContext() {
    auto& stack = GetContextStack();
    if (!stack.empty() && stack.back() == context_name_) {
        stack.pop_back();
    }
}

std::string ErrorContext::GetCurrentContext() {
    const auto& stack = GetContextStack();
    if (stack.empty()) {
        return "unknown";
    }
    
    std::string result;
    for (size_t i = 0; i < stack.size(); ++i) {
        if (i > 0) {
            result += " > ";
        }
        result += stack[i];
    }
    return result;
}

std::vector<std::string>& ErrorContext::GetContextStack() {
    return g_context_stack;
}

} // namespace error
} // namespace gimp
