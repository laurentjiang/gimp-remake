/**
 * @file error_codes.h
 * @brief Standardized error codes and categories for the GIMP remake application.
 * @author Aless Tosi
 * @date 2026-01-14
 */

#pragma once

#include <string>
#include <string_view>

namespace gimp {
namespace error {

/**
 * @brief Error category enumeration
 * 
 * Categories help organize errors by subsystem and determine handling strategy.
 */
enum class ErrorCategory {
    None = 0,
    IO,                  ///< File I/O and disk operations
    Rendering,           ///< Graphics rendering and GPU operations
    InvalidArgument,     ///< Invalid function parameters or state
    Brush,               ///< Brush engine failures
    Transform,           ///< Image transformation failures
    Filter,              ///< Filter application failures
    Memory,              ///< Memory allocation and management
    System,              ///< System-level errors (OS, drivers)
    Unknown              ///< Unclassified errors
};

/**
 * @brief Error severity level
 * 
 * Determines whether an error is recoverable or requires application termination.
 */
enum class ErrorSeverity {
    Info,        ///< Informational, no error
    Warning,     ///< Warning, operation may continue with degraded behavior
    Recoverable, ///< Error occurred but application can recover
    Fatal        ///< Fatal error, application cannot continue safely
};

/**
 * @brief Standardized error codes
 * 
 * Each error code is uniquely identified and categorized. Error codes are
 * grouped by category for easy maintenance.
 */
enum class ErrorCode {
    // Success
    Success = 0,

    // I/O Errors (1000-1999)
    IOFileNotFound = 1000,
    IOFileAccessDenied = 1001,
    IOReadError = 1002,
    IOWriteError = 1003,
    IOCorruptedFile = 1004,
    IOUnsupportedFormat = 1005,
    IODiskFull = 1006,
    IOPathTooLong = 1007,
    IOInvalidPath = 1008,
    IOFileAlreadyExists = 1009,
    IODirectoryNotFound = 1010,

    // Rendering Errors (2000-2999)
    RenderInitializationFailed = 2000,
    RenderContextLost = 2001,
    RenderShaderCompilationFailed = 2002,
    RenderTextureCreationFailed = 2003,
    RenderBufferCreationFailed = 2004,
    RenderDrawCallFailed = 2005,
    RenderUnsupportedOperation = 2006,
    RenderInvalidState = 2007,
    RenderDeviceNotFound = 2008,
    RenderDriverError = 2009,

    // Invalid Argument Errors (3000-3999)
    InvalidArgumentNull = 3000,
    InvalidArgumentOutOfRange = 3001,
    InvalidArgumentFormat = 3002,
    InvalidArgumentType = 3003,
    InvalidArgumentSize = 3004,
    InvalidArgumentCombination = 3005,
    InvalidState = 3006,
    InvalidOperation = 3007,

    // Brush Errors (4000-4099)
    BrushNotFound = 4000,
    BrushLoadFailed = 4001,
    BrushInvalidSettings = 4002,
    BrushRenderFailed = 4003,
    BrushUnsupportedType = 4004,

    // Transform Errors (4100-4199)
    TransformInvalidMatrix = 4100,
    TransformInvalidDimensions = 4101,
    TransformOperationFailed = 4102,
    TransformUnsupportedMode = 4103,

    // Filter Errors (4200-4299)
    FilterNotFound = 4200,
    FilterLoadFailed = 4201,
    FilterInvalidParameters = 4202,
    FilterExecutionFailed = 4203,
    FilterUnsupportedFormat = 4204,

    // Memory Errors (5000-5999)
    OutOfMemory = 5000,
    AllocationFailed = 5001,
    BufferTooSmall = 5002,
    MemoryCorruption = 5003,
    MemoryLeakDetected = 5004,

    // System Errors (6000-6999)
    SystemInitializationFailed = 6000,
    SystemResourceUnavailable = 6001,
    SystemPermissionDenied = 6002,
    SystemDriverError = 6003,
    SystemTimeout = 6004,
    SystemUnknownError = 6999,

    // Unknown/Other
    Unknown = 9999
};

/**
 * @brief Get the category for a given error code
 * @param code The error code to categorize
 * @return The error category
 */
constexpr ErrorCategory GetErrorCategory(ErrorCode code) {
    const int codeValue = static_cast<int>(code);
    
    if (codeValue == 0) return ErrorCategory::None;
    if (codeValue >= 1000 && codeValue < 2000) return ErrorCategory::IO;
    if (codeValue >= 2000 && codeValue < 3000) return ErrorCategory::Rendering;
    if (codeValue >= 3000 && codeValue < 4000) return ErrorCategory::InvalidArgument;
    if (codeValue >= 4000 && codeValue < 4100) return ErrorCategory::Brush;
    if (codeValue >= 4100 && codeValue < 4200) return ErrorCategory::Transform;
    if (codeValue >= 4200 && codeValue < 4300) return ErrorCategory::Filter;
    if (codeValue >= 5000 && codeValue < 6000) return ErrorCategory::Memory;
    if (codeValue >= 6000 && codeValue < 7000) return ErrorCategory::System;
    
    return ErrorCategory::Unknown;
}

/**
 * @brief Get the severity for a given error code
 * @param code The error code to evaluate
 * @return The error severity
 */
constexpr ErrorSeverity GetErrorSeverity(ErrorCode code) {
    if (code == ErrorCode::Success) {
        return ErrorSeverity::Info;
    }
    
    // Fatal errors
    switch (code) {
        case ErrorCode::OutOfMemory:
        case ErrorCode::AllocationFailed:
        case ErrorCode::MemoryCorruption:
        case ErrorCode::RenderInitializationFailed:
        case ErrorCode::RenderDeviceNotFound:
        case ErrorCode::SystemInitializationFailed:
        case ErrorCode::SystemDriverError:
            return ErrorSeverity::Fatal;
        
        // Warnings
        case ErrorCode::MemoryLeakDetected:
        case ErrorCode::SystemTimeout:
            return ErrorSeverity::Warning;
        
        // All others are recoverable
        default:
            return ErrorSeverity::Recoverable;
    }
}

/**
 * @brief Get a human-readable description of an error code
 * @param code The error code
 * @return A string describing the error
 */
inline std::string GetErrorDescription(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:
            return "Operation completed successfully";
        
        // I/O Errors
        case ErrorCode::IOFileNotFound:
            return "File not found";
        case ErrorCode::IOFileAccessDenied:
            return "File access denied";
        case ErrorCode::IOReadError:
            return "Error reading from file";
        case ErrorCode::IOWriteError:
            return "Error writing to file";
        case ErrorCode::IOCorruptedFile:
            return "File is corrupted or invalid";
        case ErrorCode::IOUnsupportedFormat:
            return "File format is not supported";
        case ErrorCode::IODiskFull:
            return "Disk is full";
        case ErrorCode::IOPathTooLong:
            return "File path is too long";
        case ErrorCode::IOInvalidPath:
            return "Invalid file path";
        case ErrorCode::IOFileAlreadyExists:
            return "File already exists";
        case ErrorCode::IODirectoryNotFound:
            return "Directory not found";
        
        // Rendering Errors
        case ErrorCode::RenderInitializationFailed:
            return "Failed to initialize rendering system";
        case ErrorCode::RenderContextLost:
            return "Rendering context was lost";
        case ErrorCode::RenderShaderCompilationFailed:
            return "Shader compilation failed";
        case ErrorCode::RenderTextureCreationFailed:
            return "Failed to create texture";
        case ErrorCode::RenderBufferCreationFailed:
            return "Failed to create buffer";
        case ErrorCode::RenderDrawCallFailed:
            return "Rendering draw call failed";
        case ErrorCode::RenderUnsupportedOperation:
            return "Rendering operation not supported";
        case ErrorCode::RenderInvalidState:
            return "Invalid rendering state";
        case ErrorCode::RenderDeviceNotFound:
            return "Rendering device not found";
        case ErrorCode::RenderDriverError:
            return "Graphics driver error";
        
        // Invalid Argument Errors
        case ErrorCode::InvalidArgumentNull:
            return "Null argument provided";
        case ErrorCode::InvalidArgumentOutOfRange:
            return "Argument out of valid range";
        case ErrorCode::InvalidArgumentFormat:
            return "Invalid argument format";
        case ErrorCode::InvalidArgumentType:
            return "Invalid argument type";
        case ErrorCode::InvalidArgumentSize:
            return "Invalid argument size";
        case ErrorCode::InvalidArgumentCombination:
            return "Invalid combination of arguments";
        case ErrorCode::InvalidState:
            return "Invalid operation state";
        case ErrorCode::InvalidOperation:
            return "Invalid operation";
        
        // Brush Errors
        case ErrorCode::BrushNotFound:
            return "Brush not found";
        case ErrorCode::BrushLoadFailed:
            return "Failed to load brush";
        case ErrorCode::BrushInvalidSettings:
            return "Invalid brush settings";
        case ErrorCode::BrushRenderFailed:
            return "Brush rendering failed";
        case ErrorCode::BrushUnsupportedType:
            return "Brush type not supported";
        
        // Transform Errors
        case ErrorCode::TransformInvalidMatrix:
            return "Invalid transformation matrix";
        case ErrorCode::TransformInvalidDimensions:
            return "Invalid transformation dimensions";
        case ErrorCode::TransformOperationFailed:
            return "Transformation operation failed";
        case ErrorCode::TransformUnsupportedMode:
            return "Transformation mode not supported";
        
        // Filter Errors
        case ErrorCode::FilterNotFound:
            return "Filter not found";
        case ErrorCode::FilterLoadFailed:
            return "Failed to load filter";
        case ErrorCode::FilterInvalidParameters:
            return "Invalid filter parameters";
        case ErrorCode::FilterExecutionFailed:
            return "Filter execution failed";
        case ErrorCode::FilterUnsupportedFormat:
            return "Filter does not support this format";
        
        // Memory Errors
        case ErrorCode::OutOfMemory:
            return "Out of memory";
        case ErrorCode::AllocationFailed:
            return "Memory allocation failed";
        case ErrorCode::BufferTooSmall:
            return "Buffer too small";
        case ErrorCode::MemoryCorruption:
            return "Memory corruption detected";
        case ErrorCode::MemoryLeakDetected:
            return "Memory leak detected";
        
        // System Errors
        case ErrorCode::SystemInitializationFailed:
            return "System initialization failed";
        case ErrorCode::SystemResourceUnavailable:
            return "System resource unavailable";
        case ErrorCode::SystemPermissionDenied:
            return "Permission denied";
        case ErrorCode::SystemDriverError:
            return "System driver error";
        case ErrorCode::SystemTimeout:
            return "Operation timed out";
        case ErrorCode::SystemUnknownError:
            return "Unknown system error";
        
        // Unknown
        case ErrorCode::Unknown:
        default:
            return "Unknown error";
    }
}

/**
 * @brief Get a human-readable name for an error category
 * @param category The error category
 * @return A string naming the category
 */
inline std::string_view GetCategoryName(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::None: return "None";
        case ErrorCategory::IO: return "I/O";
        case ErrorCategory::Rendering: return "Rendering";
        case ErrorCategory::InvalidArgument: return "InvalidArgument";
        case ErrorCategory::Brush: return "Brush";
        case ErrorCategory::Transform: return "Transform";
        case ErrorCategory::Filter: return "Filter";
        case ErrorCategory::Memory: return "Memory";
        case ErrorCategory::System: return "System";
        case ErrorCategory::Unknown: return "Unknown";
        default: return "Unknown";
    }
}

/**
 * @brief Get a human-readable name for an error severity
 * @param severity The error severity
 * @return A string naming the severity
 */
inline std::string_view GetSeverityName(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Info: return "Info";
        case ErrorSeverity::Warning: return "Warning";
        case ErrorSeverity::Recoverable: return "Recoverable";
        case ErrorSeverity::Fatal: return "Fatal";
        default: return "Unknown";
    }
}

} // namespace error
} // namespace gimp
