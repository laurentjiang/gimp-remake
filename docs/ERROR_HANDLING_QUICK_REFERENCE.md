# Error Handling Quick Reference

Quick reference for the GIMP Remake error handling system.

## Headers to Include

```cpp
#include "error_handling/error_codes.h"       // Error codes
#include "error_handling/error_result.h"      // Result<T>
#include "error_handling/error_handler.h"     // Error handler
#include "error_handling/exceptions.h"        // Exceptions
```

## Error Categories

| Category | Code Range | Examples |
|----------|------------|----------|
| I/O | 1000-1999 | File not found, access denied, corrupt file |
| Rendering | 2000-2999 | GPU init failed, shader compilation, driver error |
| InvalidArgument | 3000-3999 | Null pointer, out of range, invalid type |
| Brush | 4000-4099 | Brush not found, invalid settings |
| Transform | 4100-4199 | Invalid matrix, dimension error |
| Filter | 4200-4299 | Filter not found, invalid parameters |
| Memory | 5000-5999 | Out of memory, allocation failed |
| System | 6000-6999 | OS error, permission denied, timeout |

## Common Error Codes

```cpp
ErrorCode::Success                          // 0
ErrorCode::IOFileNotFound                   // 1000
ErrorCode::IOFileAccessDenied               // 1001
ErrorCode::IOCorruptedFile                  // 1004
ErrorCode::RenderInitializationFailed       // 2000
ErrorCode::InvalidArgumentNull              // 3000
ErrorCode::InvalidArgumentOutOfRange        // 3001
ErrorCode::OutOfMemory                      // 5000
ErrorCode::AllocationFailed                 // 5001
```

## Using Result<T>

### Return a Result

```cpp
Result<int> Divide(int a, int b) {
    if (b == 0) return ErrorCode::InvalidArgumentOutOfRange;
    return a / b;
}
```

### Check and Use Result

```cpp
auto result = Divide(10, 2);
if (result.IsOk()) {
    std::cout << result.Value();
} else {
    std::cerr << result.Error().GetMessage();
}
```

### Result<void>

```cpp
Result<void> SaveFile(const std::string& path) {
    if (path.empty()) return Err(ErrorCode::InvalidArgumentNull);
    // ... save
    return Ok();
}
```

## Using Exceptions

### Throw an Exception

```cpp
if (name.empty()) {
    THROW_EXCEPTION(InvalidArgumentException::NullArgument("name"));
}
```

### Catch Exceptions

```cpp
try {
    LoadBrush("watercolor");
} catch (const BrushException& e) {
    std::cerr << e.what();
}
```

### Available Exception Types

```cpp
IOException::FileNotFound(filename)
IOException::AccessDenied(filename)
IOException::UnsupportedFormat(format)

RenderException::InitializationFailed(context)
RenderException::ShaderCompilationFailed(shader_name)

InvalidArgumentException::NullArgument(param_name)
InvalidArgumentException::OutOfRange(param_name, value)

BrushException::NotFound(brush_name)
TransformException::InvalidMatrix(context)
FilterException::NotFound(filter_name)
FilterException::InvalidParameters(context)

MemoryException::OutOfMemory(size)
MemoryException::AllocationFailed(context)
```

## Error Reporting

### Report Error

```cpp
ErrorHandler::GetInstance().ReportError(
    ErrorCode::IOFileNotFound, 
    "config.ini"
);
```

### Report with Macro (includes context)

```cpp
REPORT_ERROR(ErrorCode::InvalidState, "Image not ready");
```

### Fatal Error

```cpp
FATAL_ERROR(ErrorCode::OutOfMemory, "Failed to allocate texture");
// Never returns - app terminates safely
```

## Error Context

### Add Context to Function

```cpp
void ProcessImage() {
    ERROR_CONTEXT("ProcessImage");
    // Errors here include "ProcessImage" in context
}
```

### Nested Context

```cpp
void Outer() {
    ERROR_CONTEXT("Outer");
    Inner();  // Context: "Outer > Inner"
}

void Inner() {
    ERROR_CONTEXT("Inner");
    // ...
}
```

## Recovery Handlers

### Register Recovery Handler

```cpp
ErrorHandler::GetInstance().RegisterRecoveryHandler(
    ErrorCategory::Rendering,
    [](const ErrorInfo& error) -> bool {
        // Try to recover
        return ReinitializeRenderer();
    }
);
```

## Error Callbacks

### Register Callback

```cpp
ErrorHandler::GetInstance().RegisterCallback(
    [](const ErrorInfo& error) {
        StatusBar::ShowError(error.GetMessage());
    }
);
```

### Category-Specific Callback

```cpp
ErrorHandler::GetInstance().RegisterCallback(
    [](const ErrorInfo& error) {
        LogIOError(error);
    },
    ErrorCategory::IO
);
```

## Initialization

### In main()

```cpp
int main(int argc, char* argv[]) {
    // Initialize first
    ErrorHandler::GetInstance().Initialize();
    
    try {
        RunApplication(argc, argv);
    } catch (const GimpException& e) {
        if (e.IsFatal()) {
            ErrorHandler::GetInstance().HandleFatalError(e.GetErrorInfo());
        }
    }
    
    // Shutdown last
    ErrorHandler::GetInstance().Shutdown();
}
```

## Configuration

```cpp
// Set log file path
ErrorHandler::GetInstance().SetLogFilePath("app_errors.log");

// Enable/disable error dialogs
ErrorHandler::GetInstance().SetShowErrorDialogs(true);

// Get error history
auto history = ErrorHandler::GetInstance().GetErrorHistory();

// Clear history
ErrorHandler::GetInstance().ClearHistory();
```

## Testing

```cpp
// Test Result<T>
auto result = Divide(10, 0);
REQUIRE(result.IsError());
REQUIRE(result.Error().GetCode() == ErrorCode::InvalidArgumentOutOfRange);

// Test Exceptions
REQUIRE_THROWS_AS(LoadBrush(""), InvalidArgumentException);
```

## When to Use What

| Scenario | Use |
|----------|-----|
| Expected errors (file not found) | **Result<T>** |
| Validation failures | **Result<T>** |
| Truly exceptional conditions | **Exception** |
| Constructor failures | **Exception** |
| Memory allocation failures | **Exception** |
| Just logging/tracking | **ReportError()** |
| Warnings | **ReportError()** |
| Fatal errors | **FATAL_ERROR()** |

## Common Patterns

### File I/O

```cpp
Result<Data> LoadFile(const std::string& path) {
    if (!exists(path)) {
        return ErrorCode::IOFileNotFound;
    }
    // ... load
    return data;
}
```

### Validation

```cpp
Result<void> Validate(int value) {
    if (value < 0) {
        return Err(ErrorCode::InvalidArgumentOutOfRange);
    }
    return Ok();
}
```

### Resource Allocation

```cpp
class Resource {
public:
    Resource(size_t size) {
        data_ = new(std::nothrow) uint8_t[size];
        if (!data_) {
            THROW_EXCEPTION(MemoryException::OutOfMemory(size));
        }
    }
};
```

## Error Severity Levels

| Level | Meaning | Action |
|-------|---------|--------|
| Info | Informational | Log only |
| Warning | Non-critical issue | Log, continue |
| Recoverable | Error but can recover | Log, try recovery, notify user |
| Fatal | Critical error | Emergency save, terminate |

## See Also

- [ERROR_HANDLING_ARCHITECTURE.md](ERROR_HANDLING_ARCHITECTURE.md) - Detailed architecture
- [ERROR_HANDLING_GUIDE.md](ERROR_HANDLING_GUIDE.md) - Comprehensive guide with examples
