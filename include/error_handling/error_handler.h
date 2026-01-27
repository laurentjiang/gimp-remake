/**
 * @file error_handler.h
 * @brief Application-wide error handler with crash handling and recovery
 * @author Laurent Jiang
 * @date 2026-01-14
 *
 * This file provides the central error handling system for the application.
 * It manages fatal vs recoverable errors, provides crash handling, and
 * coordinates error recovery strategies.
 */

#pragma once

#include "error_codes.h"
#include "error_result.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace gimp {
namespace error {

/**
 * @brief Callback function type for error notifications
 *
 * Error callbacks are invoked when errors occur, allowing different parts
 * of the application to respond to errors appropriately.
 */
using ErrorCallback = std::function<void(const ErrorInfo&)>;

/**
 * @brief Callback function type for recovery operations
 *
 * Recovery callbacks attempt to recover from specific error conditions.
 * They return true if recovery was successful, false otherwise.
 */
using RecoveryCallback = std::function<bool(const ErrorInfo&)>;

/**
 * @brief Application-wide error handler singleton
 *
 * The ErrorHandler manages all error reporting, logging, and recovery
 * throughout the application. It provides:
 * - Centralized error logging
 * - Fatal vs recoverable error handling
 * - Crash handling and safe shutdown
 * - Error recovery mechanisms
 * - Error callback registration
 */
class ErrorHandler {
  public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the ErrorHandler instance
     */
    static ErrorHandler& GetInstance();

    // Prevent copying and moving
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    ErrorHandler(ErrorHandler&&) = delete;
    ErrorHandler& operator=(ErrorHandler&&) = delete;

    /**
     * @brief Initialize the error handler
     *
     * This should be called early in application startup to set up
     * crash handlers and error logging.
     */
    void Initialize();

    /**
     * @brief Shutdown the error handler
     *
     * Flushes any pending error logs and cleans up resources.
     */
    void Shutdown();

    /**
     * @brief Report an error
     *
     * This is the main entry point for error reporting. Based on the
     * error severity, it will either:
     * - Log the error (Info/Warning)
     * - Attempt recovery (Recoverable)
     * - Initiate safe shutdown (Fatal)
     *
     * @param error The error to report
     */
    void ReportError(const ErrorInfo& error);

    /**
     * @brief Report an error from an error code
     * @param code The error code
     * @param context Additional context (optional)
     */
    void ReportError(ErrorCode code, const std::string& context = "");

    /**
     * @brief Handle a fatal error
     *
     * This will attempt to save any unsaved work, flush logs, and
     * terminate the application safely.
     *
     * @param error The fatal error information
     */
    [[noreturn]] void HandleFatalError(const ErrorInfo& error);

    /**
     * @brief Attempt to recover from an error
     *
     * Invokes registered recovery callbacks for the error category.
     *
     * @param error The error to recover from
     * @return true if recovery was successful, false otherwise
     */
    bool AttemptRecovery(const ErrorInfo& error);

    /**
     * @brief Register an error callback
     *
     * Callbacks are invoked whenever an error is reported, allowing
     * different parts of the application to respond to errors.
     *
     * @param callback The callback function
     * @param category Only invoke for errors in this category (optional)
     */
    void RegisterCallback(ErrorCallback callback, ErrorCategory category = ErrorCategory::Unknown);

    /**
     * @brief Register a recovery handler
     *
     * Recovery handlers are invoked when attempting to recover from
     * errors in a specific category.
     *
     * @param category The error category to handle
     * @param callback The recovery callback
     */
    void RegisterRecoveryHandler(ErrorCategory category, RecoveryCallback callback);

    /**
     * @brief Get the last reported error
     * @return The most recent error, or a success ErrorInfo if none
     */
    ErrorInfo GetLastError() const;

    /**
     * @brief Get all errors reported since initialization
     * @return Vector of all reported errors
     */
    std::vector<ErrorInfo> GetErrorHistory() const;

    /**
     * @brief Clear error history
     */
    void ClearHistory();

    /**
     * @brief Set whether to show error dialogs to the user
     * @param show true to show dialogs, false to suppress
     */
    void SetShowErrorDialogs(bool show);

    /**
     * @brief Check if error dialogs are enabled
     * @return true if dialogs will be shown
     */
    bool GetShowErrorDialogs() const;

    /**
     * @brief Set the error log file path
     * @param path Path to the error log file
     */
    void SetLogFilePath(const std::string& path);

    /**
     * @brief Get the error log file path
     * @return Path to the error log file
     */
    std::string GetLogFilePath() const;

  private:
    ErrorHandler();
    ~ErrorHandler();

    /**
     * @brief Write error to log file
     * @param error The error to log
     */
    void LogError(const ErrorInfo& error);

    /**
     * @brief Show error dialog to user (if enabled)
     * @param error The error to display
     */
    void ShowErrorDialog(const ErrorInfo& error);

    /**
     * @brief Perform emergency save of unsaved work
     *
     * Called before fatal error shutdown to try to save user data.
     */
    void PerformEmergencySave();

    /**
     * @brief Flush all pending log entries
     */
    void FlushLogs();

    // Member variables
    mutable std::mutex mutex_;
    bool initialized_;
    bool show_error_dialogs_;
    std::string log_file_path_;
    std::vector<ErrorInfo> error_history_;

    // Callbacks organized by category
    std::vector<std::pair<ErrorCategory, ErrorCallback>> callbacks_;
    std::vector<std::pair<ErrorCategory, RecoveryCallback>> recovery_handlers_;
};

/**
 * @brief RAII helper for error context
 *
 * Provides additional context for errors that occur within a scope.
 * Useful for adding function names, operation descriptions, etc.
 */
class ErrorContext {
  public:
    /**
     * @brief Create an error context
     * @param contextName Name of this context (e.g., function name)
     */
    explicit ErrorContext(std::string contextName);

    /**
     * @brief Destructor removes this context from the stack
     */
    ~ErrorContext();

    /**
     * @brief Get the current context stack as a string
     * @return Formatted context string
     */
    static std::string GetCurrentContext();

  private:
    static std::vector<std::string>& GetContextStack();
    std::string context_name_;
};

/**
 * @brief Macro for adding error context
 *
 * Usage: ERROR_CONTEXT("MyFunction")
 * This will add "MyFunction" to the error context for the current scope.
 */
#define ERROR_CONTEXT(name) gimp::error::ErrorContext _error_context_(name)

/**
 * @brief Macro for reporting errors with automatic context
 *
 * Usage: REPORT_ERROR(ErrorCode::IOFileNotFound, "config.ini")
 */
#define REPORT_ERROR(code, context)                                              \
    gimp::error::ErrorHandler::GetInstance().ReportError(gimp::error::ErrorInfo( \
        code, std::string(context) + " [" + gimp::error::ErrorContext::GetCurrentContext() + "]"))

/**
 * @brief Macro for handling fatal errors with automatic context
 *
 * Usage: FATAL_ERROR(ErrorCode::OutOfMemory, "Failed to allocate texture")
 */
#define FATAL_ERROR(code, context)                                                    \
    gimp::error::ErrorHandler::GetInstance().HandleFatalError(gimp::error::ErrorInfo( \
        code, std::string(context) + " [" + gimp::error::ErrorContext::GetCurrentContext() + "]"))

}  // namespace error
}  // namespace gimp
