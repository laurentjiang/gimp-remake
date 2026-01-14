/**
 * @file error_result.h
 * @brief Result<T> type for error-aware return values
 * @author Aless Tosi
 * @date 2026-01-14
 */

#pragma once

#include "error_codes.h"
#include <variant>
#include <string>
#include <optional>

namespace gimp {
namespace error {

/**
 * @brief Error information container
 * 
 * Holds detailed information about an error, including the error code,
 * category, severity, and optional context message.
 */
class ErrorInfo {
public:
    /**
     * @brief Construct an ErrorInfo for a successful operation
     */
    ErrorInfo() 
        : code_(ErrorCode::Success)
        , category_(ErrorCategory::None)
        , severity_(ErrorSeverity::Info)
        , message_("Success") {}
    
    /**
     * @brief Construct an ErrorInfo from an error code
     * @param code The error code
     * @param context Additional context about the error (optional)
     */
    explicit ErrorInfo(ErrorCode code, const std::string& context = "")
        : code_(code)
        , category_(GetErrorCategory(code))
        , severity_(GetErrorSeverity(code))
        , message_(GetErrorDescription(code)) {
        if (!context.empty()) {
            message_ += ": " + context;
        }
    }
    
    /**
     * @brief Construct an ErrorInfo with custom categorization
     * @param code The error code
     * @param category The error category
     * @param severity The error severity
     * @param message Custom error message
     */
    ErrorInfo(ErrorCode code, ErrorCategory category, ErrorSeverity severity, 
              const std::string& message)
        : code_(code)
        , category_(category)
        , severity_(severity)
        , message_(message) {}
    
    // Accessors
    ErrorCode GetCode() const { return code_; }
    ErrorCategory GetCategory() const { return category_; }
    ErrorSeverity GetSeverity() const { return severity_; }
    const std::string& GetMessage() const { return message_; }
    
    /**
     * @brief Check if this represents a successful operation
     * @return true if no error occurred
     */
    bool IsSuccess() const { return code_ == ErrorCode::Success; }
    
    /**
     * @brief Check if this error is fatal
     * @return true if the error is fatal
     */
    bool IsFatal() const { return severity_ == ErrorSeverity::Fatal; }
    
    /**
     * @brief Check if this error is recoverable
     * @return true if the error is recoverable
     */
    bool IsRecoverable() const { return severity_ == ErrorSeverity::Recoverable; }
    
    /**
     * @brief Get a formatted error string
     * @return Formatted error message with category and severity
     */
    std::string ToString() const {
        if (IsSuccess()) {
            return message_;
        }
        
        std::string result = "[";
        result += GetSeverityName(severity_);
        result += "] ";
        result += GetCategoryName(category_);
        result += " Error (";
        result += std::to_string(static_cast<int>(code_));
        result += "): ";
        result += message_;
        return result;
    }

private:
    ErrorCode code_;
    ErrorCategory category_;
    ErrorSeverity severity_;
    std::string message_;
};

/**
 * @brief Result<T> type for error-aware return values
 * 
 * A Result<T> can hold either a value of type T (success) or an ErrorInfo (failure).
 * This allows functions to return errors without throwing exceptions.
 * 
 * Example usage:
 * @code
 * Result<int> Divide(int a, int b) {
 *     if (b == 0) {
 *         return ErrorInfo(ErrorCode::InvalidArgumentOutOfRange, "Division by zero");
 *     }
 *     return a / b;
 * }
 * 
 * auto result = Divide(10, 2);
 * if (result.IsOk()) {
 *     std::cout << "Result: " << result.Value() << std::endl;
 * } else {
 *     std::cerr << "Error: " << result.Error().GetMessage() << std::endl;
 * }
 * @endcode
 */
template<typename T>
class Result {
public:
    /**
     * @brief Construct a successful Result with a value
     * @param value The success value
     */
    Result(T value) : data_(std::move(value)) {}
    
    /**
     * @brief Construct a failed Result with an error
     * @param error The error information
     */
    Result(ErrorInfo error) : data_(std::move(error)) {}
    
    /**
     * @brief Construct a failed Result from an error code
     * @param code The error code
     * @param context Additional context (optional)
     */
    Result(ErrorCode code, const std::string& context = "")
        : data_(ErrorInfo(code, context)) {}
    
    /**
     * @brief Check if the Result contains a value (success)
     * @return true if the operation was successful
     */
    bool IsOk() const {
        return std::holds_alternative<T>(data_);
    }
    
    /**
     * @brief Check if the Result contains an error (failure)
     * @return true if the operation failed
     */
    bool IsError() const {
        return std::holds_alternative<ErrorInfo>(data_);
    }
    
    /**
     * @brief Get the value (assumes IsOk() is true)
     * @return Reference to the value
     * @throws std::bad_variant_access if called on an error Result
     */
    T& Value() & {
        return std::get<T>(data_);
    }
    
    /**
     * @brief Get the value (assumes IsOk() is true)
     * @return Const reference to the value
     * @throws std::bad_variant_access if called on an error Result
     */
    const T& Value() const & {
        return std::get<T>(data_);
    }
    
    /**
     * @brief Move the value out (assumes IsOk() is true)
     * @return Moved value
     * @throws std::bad_variant_access if called on an error Result
     */
    T&& Value() && {
        return std::move(std::get<T>(data_));
    }
    
    /**
     * @brief Get the error information (assumes IsError() is true)
     * @return Reference to the error information
     * @throws std::bad_variant_access if called on a success Result
     */
    const ErrorInfo& Error() const {
        return std::get<ErrorInfo>(data_);
    }
    
    /**
     * @brief Get the value or a default if error
     * @param default_value The default value to return on error
     * @return The value or default
     */
    T ValueOr(T default_value) const & {
        if (IsOk()) {
            return Value();
        }
        return default_value;
    }
    
    /**
     * @brief Get the value or a default if error (move version)
     * @param default_value The default value to return on error
     * @return The value or default
     */
    T ValueOr(T default_value) && {
        if (IsOk()) {
            return std::move(*this).Value();
        }
        return default_value;
    }
    
    /**
     * @brief Map the value if Ok, otherwise propagate error
     * @tparam F Function type
     * @param func Function to apply to the value
     * @return New Result with transformed value or same error
     */
    template<typename F>
    auto Map(F&& func) const & -> Result<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        if (IsOk()) {
            return Result<U>(func(Value()));
        }
        return Result<U>(Error());
    }
    
    /**
     * @brief Convert to bool (true if Ok)
     * @return true if the Result contains a value
     */
    explicit operator bool() const {
        return IsOk();
    }

private:
    std::variant<T, ErrorInfo> data_;
};

/**
 * @brief Specialization of Result<void> for operations that don't return a value
 * 
 * This specialization allows functions that don't return a value to still
 * use the Result pattern for error handling.
 */
template<>
class Result<void> {
public:
    /**
     * @brief Construct a successful Result<void>
     */
    Result() : error_(std::nullopt) {}
    
    /**
     * @brief Construct a failed Result<void> with an error
     * @param error The error information
     */
    Result(ErrorInfo error) : error_(std::move(error)) {}
    
    /**
     * @brief Construct a failed Result<void> from an error code
     * @param code The error code
     * @param context Additional context (optional)
     */
    Result(ErrorCode code, const std::string& context = "")
        : error_(ErrorInfo(code, context)) {}
    
    /**
     * @brief Check if the Result represents success
     * @return true if the operation was successful
     */
    bool IsOk() const {
        return !error_.has_value();
    }
    
    /**
     * @brief Check if the Result contains an error
     * @return true if the operation failed
     */
    bool IsError() const {
        return error_.has_value();
    }
    
    /**
     * @brief Get the error information (assumes IsError() is true)
     * @return Reference to the error information
     */
    const ErrorInfo& Error() const {
        return error_.value();
    }
    
    /**
     * @brief Convert to bool (true if Ok)
     * @return true if the Result represents success
     */
    explicit operator bool() const {
        return IsOk();
    }

private:
    std::optional<ErrorInfo> error_;
};

/**
 * @brief Helper function to create a successful Result<void>
 * @return A successful Result<void>
 */
inline Result<void> Ok() {
    return Result<void>();
}

/**
 * @brief Helper function to create an error Result<void>
 * @param code The error code
 * @param context Additional context (optional)
 * @return A failed Result<void>
 */
inline Result<void> Err(ErrorCode code, const std::string& context = "") {
    return Result<void>(code, context);
}

/**
 * @brief Helper function to create an error Result<void>
 * @param error The error information
 * @return A failed Result<void>
 */
inline Result<void> Err(ErrorInfo error) {
    return Result<void>(std::move(error));
}

} // namespace error
} // namespace gimp
