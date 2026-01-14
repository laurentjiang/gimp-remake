# Error Handling Architecture

## Overview

The GIMP Remake error handling system provides a comprehensive, type-safe approach to managing errors throughout the application. The architecture is designed to be simple yet robust, with clear separation between recoverable and fatal errors, and support for both exception-based and return-value-based error handling.

## Design Goals

1. **Consistency**: All errors use standardized error codes and categories
2. **Type Safety**: Compile-time checks where possible, with strong typing
3. **Clarity**: Clear distinction between recoverable and fatal errors
4. **Flexibility**: Support both exceptions and Result<T> patterns
5. **Debuggability**: Rich context and logging for troubleshooting
6. **Recoverability**: Graceful degradation and recovery mechanisms

## Architecture Components

### 1. Error Codes (`error_codes.h`)

The foundation of the error handling system is a comprehensive set of error codes organized by category:

```
ErrorCode (enum class)
├── I/O Errors (1000-1999)
│   ├── File operations
│   ├── Path errors
│   └── Format errors
├── Rendering Errors (2000-2999)
│   ├── Initialization
│   ├── GPU operations
│   └── Driver errors
├── Invalid Argument Errors (3000-3999)
│   ├── Null pointers
│   ├── Range violations
│   └── Type mismatches
├── Brush Errors (4000-4099)
├── Transform Errors (4100-4199)
├── Filter Errors (4200-4299)
├── Memory Errors (5000-5999)
│   ├── Allocation failures
│   ├── Buffer overflows
│   └── Out of memory
└── System Errors (6000-6999)
    ├── OS errors
    ├── Driver errors
    └── Resource unavailable
```

#### Error Categories

Errors are automatically categorized based on their code:
- **IO**: File and disk operations
- **Rendering**: Graphics and GPU operations
- **InvalidArgument**: Parameter validation failures
- **Brush/Transform/Filter**: Tool-specific failures
- **Memory**: Memory management issues
- **System**: OS and driver issues

#### Error Severity

Each error code has an associated severity level:
- **Info**: Informational, no error
- **Warning**: Non-critical issue, operation continues
- **Recoverable**: Error occurred but can be recovered
- **Fatal**: Critical error requiring application termination

### 2. Error Information (`error_result.h`)

#### ErrorInfo Class

Encapsulates complete error information:
- Error code
- Category (auto-derived)
- Severity (auto-derived)
- Human-readable message
- Optional context information

```cpp
ErrorInfo error(ErrorCode::IOFileNotFound, "config.ini");
std::cout << error.ToString();
// Output: [Recoverable] I/O Error (1000): File not found: config.ini
```

#### Result<T> Type

A modern error handling pattern that returns either a value or an error:

```cpp
template<typename T>
class Result {
    // Either holds T (success) or ErrorInfo (failure)
};
```

Benefits:
- **Explicit error handling**: Caller must check for errors
- **No exceptions**: Predictable control flow
- **Type safe**: Compile-time checking
- **Composable**: Can chain operations with map()

### 3. Error Handler (`error_handler.h/cpp`)

The central error management system, implemented as a singleton.

#### Key Responsibilities

1. **Error Reporting**: Centralized error logging and notification
2. **Severity Handling**: Different actions based on error severity
3. **Recovery**: Attempt recovery for recoverable errors
4. **Fatal Error Management**: Safe shutdown on fatal errors
5. **Logging**: File-based error logging with timestamps
6. **User Notification**: Optional error dialogs

#### Error Flow

```
Error Occurs
    ↓
ErrorHandler::ReportError()
    ↓
Add to history
    ↓
Log to file
    ↓
Invoke callbacks
    ↓
┌─────────────────────────┐
│ Check Severity          │
├─────────────────────────┤
│ Info/Warning: Log only  │
│ Recoverable: Try recover│
│ Fatal: Safe shutdown    │
└─────────────────────────┘
```

#### Recovery Mechanism

Recovery handlers can be registered for specific error categories:

```cpp
ErrorHandler::GetInstance().RegisterRecoveryHandler(
    ErrorCategory::Rendering,
    [](const ErrorInfo& error) -> bool {
        // Attempt to recover from rendering errors
        return ReinitializeRenderer();
    }
);
```

#### Crash Handling

For fatal errors:
1. Emergency save of all open documents
2. Flush error logs
3. Display error dialog to user
4. Graceful application termination

### 4. Exception Classes (`exceptions.h`)

Traditional exception-based error handling for situations where exceptions are preferred.

#### Exception Hierarchy

```
std::exception
    ↓
GimpException (base)
    ├── IOException
    ├── RenderException
    ├── InvalidArgumentException
    ├── BrushException
    ├── TransformException
    ├── FilterException
    └── MemoryException
```

All exceptions wrap `ErrorInfo` and automatically integrate with the error handler.

#### Static Factory Methods

Convenient exception creation:

```cpp
throw IOException::FileNotFound("config.ini");
throw RenderException::ShaderCompilationFailed("blur.glsl");
throw MemoryException::OutOfMemory(1024 * 1024);
```

### 5. Error Context (`error_handler.h`)

Thread-local error context stack for debugging:

```cpp
void ProcessImage() {
    ERROR_CONTEXT("ProcessImage");
    // Errors here will include "ProcessImage" in context
    
    ApplyFilter();  // Adds another level to context
}

void ApplyFilter() {
    ERROR_CONTEXT("ApplyFilter");
    // Context is now: "ProcessImage > ApplyFilter"
}
```

## Error Handling Strategies

### Strategy 1: Result<T> (Recommended for Expected Errors)

Use when errors are expected and should be handled locally:

```cpp
Result<Image> LoadImage(const std::string& path) {
    if (!FileExists(path)) {
        return ErrorCode::IOFileNotFound;
    }
    // ... load image
    return image;
}

// Usage
auto result = LoadImage("photo.png");
if (result.IsOk()) {
    ProcessImage(result.Value());
} else {
    ErrorHandler::GetInstance().ReportError(result.Error());
    // Handle error
}
```

**When to use:**
- File I/O operations
- Validation functions
- Operations where failure is common
- Functions that don't want to throw

### Strategy 2: Exceptions (For Unexpected Errors)

Use for truly exceptional conditions that shouldn't occur in normal operation:

```cpp
void AllocateBuffer(size_t size) {
    void* buffer = malloc(size);
    if (!buffer) {
        THROW_EXCEPTION(MemoryException::OutOfMemory(size));
    }
}

// Usage
try {
    AllocateBuffer(huge_size);
} catch (const MemoryException& e) {
    // Handle memory error
}
```

**When to use:**
- Memory allocation failures
- Invalid program state
- Unrecoverable errors
- Constructor failures (can't return Result)

### Strategy 3: Direct Error Handler (For Logging)

Use for non-critical errors that should be logged but not interrupt flow:

```cpp
void OptimizeImage() {
    if (!CanOptimize()) {
        REPORT_ERROR(ErrorCode::InvalidState, "Image not optimizable");
        return;  // Continue execution
    }
    // ... optimize
}
```

**When to use:**
- Warnings
- Performance issues
- Deprecated functionality
- Informational messages

## Integration Points

### Application Startup

```cpp
int main(int argc, char* argv[]) {
    // Initialize error handler early
    ErrorHandler::GetInstance().Initialize();
    
    // Set up recovery handlers
    RegisterRecoveryHandlers();
    
    // Run application with top-level exception handler
    try {
        return RunApplication(argc, argv);
    } catch (const GimpException& e) {
        if (e.GetSeverity() == ErrorSeverity::Fatal) {
            ErrorHandler::GetInstance().HandleFatalError(e.GetErrorInfo());
        }
    }
    
    ErrorHandler::GetInstance().Shutdown();
}
```

### Subsystem Integration

Each subsystem should:
1. Define its error codes in the appropriate range
2. Register recovery handlers for its errors
3. Use consistent error reporting
4. Document expected vs. exceptional errors

## Testing Strategy

### Unit Testing

```cpp
TEST_CASE("Error handling") {
    ErrorHandler::GetInstance().ClearHistory();
    
    // Test error reporting
    ErrorHandler::GetInstance().ReportError(
        ErrorCode::IOFileNotFound, "test.txt"
    );
    
    auto history = ErrorHandler::GetInstance().GetErrorHistory();
    REQUIRE(history.size() == 1);
    REQUIRE(history[0].GetCode() == ErrorCode::IOFileNotFound);
}
```

### Integration Testing

- Test recovery mechanisms
- Verify fatal error handling
- Test error propagation
- Validate logging behavior

## Performance Considerations

1. **Result<T>**: Zero-cost abstraction when successful
2. **Error Codes**: Compile-time constants
3. **Severity/Category**: Computed at compile time (constexpr)
4. **Logging**: Buffered writes, async where possible
5. **Context Stack**: Thread-local, minimal overhead

## Future Enhancements

1. **Stack Traces**: Capture call stacks for fatal errors
2. **Telemetry**: Optional error reporting to developers
3. **Localization**: Translated error messages
4. **Error Statistics**: Track error frequency and patterns
5. **Debug Symbols**: Enhanced error context in debug builds
6. **Platform-Specific Handlers**: OS-specific crash handling

## Summary

The error handling architecture provides:
- ✅ Standardized error codes and categories
- ✅ Clear severity levels (fatal vs. recoverable)
- ✅ Both exception and Result<T> patterns
- ✅ Centralized error management
- ✅ Recovery mechanisms
- ✅ Crash handling and safe shutdown
- ✅ Rich debugging context
- ✅ Comprehensive logging

This architecture ensures consistent, debuggable, and maintainable error handling across the entire application.
