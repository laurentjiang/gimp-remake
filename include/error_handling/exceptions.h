/**
 * @file exceptions.h
 * @brief Custom exception classes for the GIMP remake application
 * @author Aless Tosi
 * @date 2026-01-14
 */

#pragma once

#include "error_codes.h"
#include "error_result.h"
#include <exception>
#include <string>

namespace gimp {
namespace error {

/**
 * @brief Base exception class for all GIMP remake exceptions
 * 
 * This class wraps ErrorInfo and provides standard exception interface.
 * All custom exceptions in the application should derive from this class.
 */
class GimpException : public std::exception {
public:
    /**
     * @brief Construct exception from error code
     * @param code The error code
     * @param context Additional context (optional)
     */
    explicit GimpException(ErrorCode code, const std::string& context = "")
        : error_info_(code, context) {}
    
    /**
     * @brief Construct exception from ErrorInfo
     * @param error The error information
     */
    explicit GimpException(ErrorInfo error)
        : error_info_(std::move(error)) {}
    
    /**
     * @brief Get the error message
     * @return C-string containing the error message
     */
    const char* what() const noexcept override {
        // Cache the string to ensure it remains valid
        if (cached_what_.empty()) {
            cached_what_ = error_info_.ToString();
        }
        return cached_what_.c_str();
    }
    
    /**
     * @brief Get the error information
     * @return Reference to the ErrorInfo
     */
    const ErrorInfo& GetErrorInfo() const {
        return error_info_;
    }
    
    /**
     * @brief Get the error code
     * @return The error code
     */
    ErrorCode GetCode() const {
        return error_info_.GetCode();
    }
    
    /**
     * @brief Get the error category
     * @return The error category
     */
    ErrorCategory GetCategory() const {
        return error_info_.GetCategory();
    }
    
    /**
     * @brief Get the error severity
     * @return The error severity
     */
    ErrorSeverity GetSeverity() const {
        return error_info_.GetSeverity();
    }

protected:
    ErrorInfo error_info_;
    mutable std::string cached_what_;
};

/**
 * @brief Exception for I/O errors
 * 
 * Thrown when file operations or disk access fails.
 */
class IOException : public GimpException {
public:
    explicit IOException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::IO) {
            // Ensure the error code is an I/O error
            error_info_ = ErrorInfo(ErrorCode::IOReadError, context);
        }
    }
    
    /**
     * @brief Create IOException for file not found
     * @param filename The filename that wasn't found
     * @return IOException instance
     */
    static IOException FileNotFound(const std::string& filename) {
        return IOException(ErrorCode::IOFileNotFound, filename);
    }
    
    /**
     * @brief Create IOException for access denied
     * @param filename The filename that couldn't be accessed
     * @return IOException instance
     */
    static IOException AccessDenied(const std::string& filename) {
        return IOException(ErrorCode::IOFileAccessDenied, filename);
    }
    
    /**
     * @brief Create IOException for unsupported format
     * @param format The unsupported format
     * @return IOException instance
     */
    static IOException UnsupportedFormat(const std::string& format) {
        return IOException(ErrorCode::IOUnsupportedFormat, format);
    }
};

/**
 * @brief Exception for rendering errors
 * 
 * Thrown when graphics operations fail.
 */
class RenderException : public GimpException {
public:
    explicit RenderException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::Rendering) {
            // Ensure the error code is a rendering error
            error_info_ = ErrorInfo(ErrorCode::RenderDrawCallFailed, context);
        }
    }
    
    /**
     * @brief Create RenderException for initialization failure
     * @param context Additional context
     * @return RenderException instance
     */
    static RenderException InitializationFailed(const std::string& context = "") {
        return RenderException(ErrorCode::RenderInitializationFailed, context);
    }
    
    /**
     * @brief Create RenderException for shader compilation failure
     * @param shader_name The shader that failed to compile
     * @return RenderException instance
     */
    static RenderException ShaderCompilationFailed(const std::string& shader_name) {
        return RenderException(ErrorCode::RenderShaderCompilationFailed, shader_name);
    }
};

/**
 * @brief Exception for invalid arguments
 * 
 * Thrown when function parameters are invalid.
 */
class InvalidArgumentException : public GimpException {
public:
    explicit InvalidArgumentException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::InvalidArgument) {
            // Ensure the error code is an invalid argument error
            error_info_ = ErrorInfo(ErrorCode::InvalidArgumentNull, context);
        }
    }
    
    /**
     * @brief Create InvalidArgumentException for null argument
     * @param param_name The parameter name
     * @return InvalidArgumentException instance
     */
    static InvalidArgumentException NullArgument(const std::string& param_name) {
        return InvalidArgumentException(ErrorCode::InvalidArgumentNull, param_name);
    }
    
    /**
     * @brief Create InvalidArgumentException for out of range
     * @param param_name The parameter name
     * @param value The invalid value
     * @return InvalidArgumentException instance
     */
    static InvalidArgumentException OutOfRange(const std::string& param_name, 
                                               const std::string& value) {
        return InvalidArgumentException(ErrorCode::InvalidArgumentOutOfRange, 
                                       param_name + " = " + value);
    }
};

/**
 * @brief Exception for brush errors
 * 
 * Thrown when brush operations fail.
 */
class BrushException : public GimpException {
public:
    explicit BrushException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::Brush) {
            error_info_ = ErrorInfo(ErrorCode::BrushRenderFailed, context);
        }
    }
    
    /**
     * @brief Create BrushException for brush not found
     * @param brush_name The brush name
     * @return BrushException instance
     */
    static BrushException NotFound(const std::string& brush_name) {
        return BrushException(ErrorCode::BrushNotFound, brush_name);
    }
};

/**
 * @brief Exception for transform errors
 * 
 * Thrown when image transformations fail.
 */
class TransformException : public GimpException {
public:
    explicit TransformException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::Transform) {
            error_info_ = ErrorInfo(ErrorCode::TransformOperationFailed, context);
        }
    }
    
    /**
     * @brief Create TransformException for invalid matrix
     * @param context Additional context
     * @return TransformException instance
     */
    static TransformException InvalidMatrix(const std::string& context = "") {
        return TransformException(ErrorCode::TransformInvalidMatrix, context);
    }
};

/**
 * @brief Exception for filter errors
 * 
 * Thrown when filter operations fail.
 */
class FilterException : public GimpException {
public:
    explicit FilterException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::Filter) {
            error_info_ = ErrorInfo(ErrorCode::FilterExecutionFailed, context);
        }
    }
    
    /**
     * @brief Create FilterException for filter not found
     * @param filter_name The filter name
     * @return FilterException instance
     */
    static FilterException NotFound(const std::string& filter_name) {
        return FilterException(ErrorCode::FilterNotFound, filter_name);
    }
    
    /**
     * @brief Create FilterException for invalid parameters
     * @param context Parameter details
     * @return FilterException instance
     */
    static FilterException InvalidParameters(const std::string& context) {
        return FilterException(ErrorCode::FilterInvalidParameters, context);
    }
};

/**
 * @brief Exception for memory errors
 * 
 * Thrown when memory allocation or management fails.
 */
class MemoryException : public GimpException {
public:
    explicit MemoryException(ErrorCode code, const std::string& context = "")
        : GimpException(code, context) {
        if (GetErrorCategory(code) != ErrorCategory::Memory) {
            error_info_ = ErrorInfo(ErrorCode::OutOfMemory, context);
        }
    }
    
    /**
     * @brief Create MemoryException for out of memory
     * @param size_requested The size that couldn't be allocated
     * @return MemoryException instance
     */
    static MemoryException OutOfMemory(size_t size_requested) {
        return MemoryException(ErrorCode::OutOfMemory, 
                              "Requested: " + std::to_string(size_requested) + " bytes");
    }
    
    /**
     * @brief Create MemoryException for allocation failure
     * @param context Additional context
     * @return MemoryException instance
     */
    static MemoryException AllocationFailed(const std::string& context = "") {
        return MemoryException(ErrorCode::AllocationFailed, context);
    }
};

/**
 * @brief Helper macro to throw an exception and report it to the error handler
 * 
 * Usage: THROW_EXCEPTION(IOException::FileNotFound("config.ini"))
 */
#define THROW_EXCEPTION(exception) \
    do { \
        auto ex = (exception); \
        gimp::error::ErrorHandler::GetInstance().ReportError(ex.GetErrorInfo()); \
        throw ex; \
    } while (0)

/**
 * @brief Helper macro to catch and handle exceptions
 * 
 * Usage:
 * HANDLE_EXCEPTIONS({
 *     // Code that might throw
 * }, {
 *     // Recovery code
 * });
 */
#define HANDLE_EXCEPTIONS(code_block, recovery_block) \
    try { \
        code_block \
    } catch (const gimp::error::GimpException& e) { \
        gimp::error::ErrorHandler::GetInstance().ReportError(e.GetErrorInfo()); \
        recovery_block \
    } catch (const std::exception& e) { \
        gimp::error::ErrorHandler::GetInstance().ReportError( \
            gimp::error::ErrorCode::Unknown, std::string("Unhandled exception: ") + e.what()); \
        recovery_block \
    }

} // namespace error
} // namespace gimp
