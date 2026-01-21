/**
 * @file error_handler.cpp
 * @brief Implementation of the application-wide error handler
 * @author Aless Tosi
 * @date 2026-01-14
 */

#include "error_handling/error_handler.h"

#include <QApplication>
#include <QMessageBox>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace gimp::error {

// Static context stack for error context tracking
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static thread_local std::vector<std::string> gContextStack;

// ErrorHandler Implementation

ErrorHandler& ErrorHandler::GetInstance()
{
    static ErrorHandler instance;
    return instance;
}

ErrorHandler::ErrorHandler()
    : initialized_(false),
      show_error_dialogs_(true),
      log_file_path_("gimp_remake_errors.log")
{
}

// NOLINTNEXTLINE(bugprone-exception-escape)
ErrorHandler::~ErrorHandler()
{
    if (initialized_) {
        Shutdown();
    }
}

void ErrorHandler::Initialize()
{
    const std::scoped_lock lock(mutex_);

    if (initialized_) {
        return;
    }

    // Set up crash handlers (platform-specific implementation would go here)
    // For now, we just mark as initialized
    initialized_ = true;

    // Log initialization
    std::cout << "Error handler initialized. Log file: " << log_file_path_ << '\n';
}

void ErrorHandler::Shutdown()
{
    const std::scoped_lock lock(mutex_);

    if (!initialized_) {
        return;
    }

    // Flush any pending logs
    FlushLogs();

    initialized_ = false;

    std::cout << "Error handler shut down.\n";
}

void ErrorHandler::ReportError(const ErrorInfo& error)
{
    const std::scoped_lock lock(mutex_);

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
            std::cout << error.ToString() << '\n';
            break;

        case ErrorSeverity::Recoverable:
            // Try to recover
            std::cerr << error.ToString() << '\n';
            if (!AttemptRecovery(error)) {
                // Recovery failed, show error dialog if enabled
                if (show_error_dialogs_) {
                    ShowErrorDialog(error);
                }
            }
            break;

        case ErrorSeverity::Fatal:
            // Fatal errors are handled separately
            std::cerr << "FATAL: " << error.ToString() << '\n';
            // Note: HandleFatalError should be called directly for fatal errors
            break;
    }
}

void ErrorHandler::ReportError(ErrorCode code, const std::string& context)
{
    ReportError(ErrorInfo(code, context));
}

[[noreturn]] void ErrorHandler::HandleFatalError(const ErrorInfo& error)
{
    std::cerr << "========================================\n";
    std::cerr << "FATAL ERROR - APPLICATION WILL TERMINATE\n";
    std::cerr << error.ToString() << '\n';
    std::cerr << "========================================\n";

    // Log the fatal error
    LogError(error);

    // Attempt emergency save
    PerformEmergencySave();

    // Flush logs
    FlushLogs();

    // Show error dialog (even if dialogs are disabled for non-fatal errors)
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (QApplication::instance()) {
        QMessageBox::critical(
            nullptr,
            "Fatal Error",
            QString::fromStdString(
                "A fatal error has occurred and the application must close.\n\n" +
                error.ToString() + "\n\n" + "An error log has been saved to: " + log_file_path_));
    }

    // Terminate
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    std::exit(EXIT_FAILURE);
}

bool ErrorHandler::AttemptRecovery(const ErrorInfo& error)
{
    const std::scoped_lock lock(mutex_);

    // Try recovery handlers for this category
    for (const auto& [category, recoveryHandler] : recovery_handlers_) {
        if (category == error.GetCategory()) {
            if (recoveryHandler(error)) {
                std::cout << "Successfully recovered from error: " << error.GetMessage() << '\n';
                return true;
            }
        }
    }

    return false;
}

void ErrorHandler::RegisterCallback(ErrorCallback callback, ErrorCategory category)
{
    const std::scoped_lock lock(mutex_);
    callbacks_.emplace_back(category, std::move(callback));
}

void ErrorHandler::RegisterRecoveryHandler(ErrorCategory category, RecoveryCallback callback)
{
    const std::scoped_lock lock(mutex_);
    recovery_handlers_.emplace_back(category, std::move(callback));
}

ErrorInfo ErrorHandler::GetLastError() const
{
    const std::scoped_lock lock(mutex_);
    if (error_history_.empty()) {
        return {};  // Success
    }
    return error_history_.back();
}

std::vector<ErrorInfo> ErrorHandler::GetErrorHistory() const
{
    const std::scoped_lock lock(mutex_);
    return error_history_;
}

void ErrorHandler::ClearHistory()
{
    const std::scoped_lock lock(mutex_);
    error_history_.clear();
}

void ErrorHandler::SetShowErrorDialogs(bool show)
{
    const std::scoped_lock lock(mutex_);
    show_error_dialogs_ = show;
}

bool ErrorHandler::GetShowErrorDialogs() const
{
    const std::scoped_lock lock(mutex_);
    return show_error_dialogs_;
}

void ErrorHandler::SetLogFilePath(const std::string& path)
{
    const std::scoped_lock lock(mutex_);
    log_file_path_ = path;
}

std::string ErrorHandler::GetLogFilePath() const
{
    const std::scoped_lock lock(mutex_);
    return log_file_path_;
}

void ErrorHandler::LogError(const ErrorInfo& error)
{
    // Get current timestamp
    auto now = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << " | " << error.ToString() << '\n';

    // Write to log file
    std::ofstream logFile(log_file_path_, std::ios::app);
    if (logFile.is_open()) {
        logFile << oss.str();
        logFile.flush();
    }
}

void ErrorHandler::ShowErrorDialog(const ErrorInfo& error)
{
    // NOLINTNEXTLINE(readability-implicit-bool-conversion)
    if (!QApplication::instance()) {
        return;  // Can't show dialog without QApplication
    }

    QString title;
    QMessageBox::Icon icon = QMessageBox::Information;

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
            break;
    }

    QMessageBox msgBox;
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(QString::fromStdString(error.ToString()));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void ErrorHandler::PerformEmergencySave()
{
    std::cout << "Attempting emergency save...\n";

    // TODO: Implement emergency save logic
    // This would:
    // 1. Save all open documents to backup locations
    // 2. Save application state
    // 3. Save undo history if possible

    std::cout << "Emergency save completed (not implemented yet).\n";
}

void ErrorHandler::FlushLogs()
{
    // Logs are flushed after each write, so nothing to do here
    // This method exists for future expansion
}

// ErrorContext Implementation

ErrorContext::ErrorContext(std::string context_name) : context_name_(std::move(context_name))
{
    GetContextStack().push_back(context_name_);
}

ErrorContext::~ErrorContext()
{
    auto& stack = GetContextStack();
    if (!stack.empty() && stack.back() == context_name_) {
        stack.pop_back();
    }
}

std::string ErrorContext::GetCurrentContext()
{
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

std::vector<std::string>& ErrorContext::GetContextStack()
{
    return gContextStack;
}

}  // namespace gimp::error
