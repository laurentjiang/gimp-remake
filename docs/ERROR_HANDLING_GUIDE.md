# Error Handling Guide

This guide provides practical examples and best practices for using the GIMP Remake error handling system.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Choosing an Error Strategy](#choosing-an-error-strategy)
3. [Using Result<T>](#using-resultt)
4. [Using Exceptions](#using-exceptions)
5. [Error Reporting](#error-reporting)
6. [Recovery Handlers](#recovery-handlers)
7. [Error Context](#error-context)
8. [Best Practices](#best-practices)
9. [Common Patterns](#common-patterns)

## Quick Start

### Include Headers

```cpp
#include "error_handling/error_codes.h"       // Error codes and categories
#include "error_handling/error_result.h"      // Result<T> type
#include "error_handling/error_handler.h"     // Error handler singleton
#include "error_handling/exceptions.h"        // Exception classes
```

### Initialize Error Handler (in main)

```cpp
int main(int argc, char* argv[]) {
    using namespace gimp::error;
    
    // Initialize error handler
    ErrorHandler::GetInstance().Initialize();
    
    // Your application code
    
    // Shutdown error handler
    ErrorHandler::GetInstance().Shutdown();
    return 0;
}
```

## Choosing an Error Strategy

### Use Result<T> when:
- ✅ Errors are expected and common (e.g., file not found)
- ✅ Caller should handle the error locally
- ✅ You want explicit error handling
- ✅ You prefer functional-style error propagation
- ✅ Performance is critical (no exception overhead)

### Use Exceptions when:
- ✅ Errors are truly exceptional
- ✅ Errors need to propagate up multiple call levels
- ✅ You're in a constructor (can't return Result)
- ✅ The error is unrecoverable at the call site
- ✅ Existing code already uses exceptions

### Use Direct Reporting when:
- ✅ You want to log an error but continue execution
- ✅ The error is informational or a warning
- ✅ You're debugging and want to track issues

## Using Result<T>

### Basic Usage

```cpp
using namespace gimp::error;

// Function that returns Result<T>
Result<int> Divide(int a, int b) {
    if (b == 0) {
        return ErrorCode::InvalidArgumentOutOfRange;
    }
    return a / b;
}

// Checking the result
auto result = Divide(10, 2);
if (result.IsOk()) {
    std::cout << "Result: " << result.Value() << std::endl;
} else {
    std::cerr << "Error: " << result.Error().GetMessage() << std::endl;
}
```

### Result<T> with Custom Context

```cpp
Result<Image> LoadImage(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return ErrorInfo(ErrorCode::IOFileNotFound, 
                        "Path: " + path);
    }
    
    Image img;
    if (!img.Load(path)) {
        return ErrorInfo(ErrorCode::IOCorruptedFile,
                        "Failed to parse: " + path);
    }
    
    return img;
}
```

### Result<void> for Operations Without Return Value

```cpp
Result<void> SaveProject(const std::string& path) {
    if (path.empty()) {
        return Err(ErrorCode::InvalidArgumentNull, "Empty path");
    }
    
    if (!HasWritePermission(path)) {
        return Err(ErrorCode::IOFileAccessDenied, path);
    }
    
    // Save project
    return Ok();  // Success
}

// Usage
auto result = SaveProject("project.gimp");
if (!result.IsOk()) {
    ErrorHandler::GetInstance().ReportError(result.Error());
}
```

### Chaining with Map

```cpp
auto result = LoadImage("photo.png")
    .Map([](const Image& img) {
        return img.Resize(800, 600);
    })
    .Map([](const Image& img) {
        return img.ApplyFilter("blur");
    });

if (result.IsOk()) {
    DisplayImage(result.Value());
}
```

### Using ValueOr for Defaults

```cpp
int GetConfigValue(const std::string& key) {
    return ReadConfig(key).ValueOr(42);  // Default to 42 on error
}
```

## Using Exceptions

### Throwing Exceptions

```cpp
using namespace gimp::error;

void LoadBrush(const std::string& name) {
    if (name.empty()) {
        THROW_EXCEPTION(
            InvalidArgumentException::NullArgument("name")
        );
    }
    
    if (!BrushExists(name)) {
        THROW_EXCEPTION(
            BrushException::NotFound(name)
        );
    }
    
    // Load brush...
}
```

### Catching Specific Exceptions

```cpp
try {
    LoadBrush("watercolor");
    ApplyBrush();
} catch (const BrushException& e) {
    std::cerr << "Brush error: " << e.what() << std::endl;
    // Use default brush
    LoadBrush("default");
} catch (const IOException& e) {
    std::cerr << "I/O error: " << e.what() << std::endl;
    // Show error to user
    ShowErrorDialog(e.GetErrorInfo());
}
```

### Catching All GIMP Exceptions

```cpp
try {
    PerformComplexOperation();
} catch (const GimpException& e) {
    // Handle any GIMP exception
    ErrorHandler::GetInstance().ReportError(e.GetErrorInfo());
    
    if (e.GetSeverity() == ErrorSeverity::Fatal) {
        // Fatal error - must exit
        throw;
    }
    // Try to recover from non-fatal errors
}
```

### Using HANDLE_EXCEPTIONS Macro

```cpp
HANDLE_EXCEPTIONS({
    LoadProject("myproject.gimp");
    ProcessAllLayers();
    SaveProject("myproject.gimp");
}, {
    // Recovery code
    std::cerr << "Failed to process project" << std::endl;
    CreateEmergencyBackup();
});
```

## Error Reporting

### Direct Error Reporting

```cpp
using namespace gimp::error;

void ValidateSettings() {
    if (settings.brush_size < 1 || settings.brush_size > 100) {
        ErrorHandler::GetInstance().ReportError(
            ErrorCode::InvalidArgumentOutOfRange,
            "Brush size must be 1-100, got: " + 
            std::to_string(settings.brush_size)
        );
    }
}
```

### Using the REPORT_ERROR Macro (with context)

```cpp
void ProcessImage() {
    ERROR_CONTEXT("ProcessImage");
    
    if (!image.IsValid()) {
        REPORT_ERROR(ErrorCode::InvalidState, "Invalid image state");
        return;
    }
    
    // Process...
}
// Error will include context: "ProcessImage"
```

### Reporting Fatal Errors

```cpp
void InitializeRenderer() {
    if (!CreateGLContext()) {
        // Fatal error - can't continue without renderer
        FATAL_ERROR(
            ErrorCode::RenderInitializationFailed,
            "Failed to create OpenGL context"
        );
        // This never returns - app will terminate safely
    }
}
```

## Recovery Handlers

### Registering a Recovery Handler

```cpp
void SetupErrorHandlers() {
    using namespace gimp::error;
    
    // Register recovery for rendering errors
    ErrorHandler::GetInstance().RegisterRecoveryHandler(
        ErrorCategory::Rendering,
        [](const ErrorInfo& error) -> bool {
            std::cout << "Attempting to recover from: " 
                     << error.GetMessage() << std::endl;
            
            // Try to reinitialize renderer
            if (ReinitializeRenderer()) {
                std::cout << "Renderer recovered successfully" << std::endl;
                return true;
            }
            
            return false;  // Recovery failed
        }
    );
}
```

### Recovery for I/O Errors

```cpp
ErrorHandler::GetInstance().RegisterRecoveryHandler(
    ErrorCategory::IO,
    [](const ErrorInfo& error) -> bool {
        // For I/O errors, try alternate locations
        if (error.GetCode() == ErrorCode::IOFileNotFound) {
            return TryAlternateLocations();
        }
        
        // For access denied, prompt for elevated permissions
        if (error.GetCode() == ErrorCode::IOFileAccessDenied) {
            return RequestElevatedPermissions();
        }
        
        return false;
    }
);
```

### Recovery for Memory Errors

```cpp
ErrorHandler::GetInstance().RegisterRecoveryHandler(
    ErrorCategory::Memory,
    [](const ErrorInfo& error) -> bool {
        // Try to free some memory
        ClearImageCache();
        ClearUndoHistory();
        
        // Force garbage collection
        RunGarbageCollection();
        
        // If we freed enough memory, recovery succeeded
        return GetAvailableMemory() > GetMinimumRequiredMemory();
    }
);
```

## Error Context

### Adding Context to Function Calls

```cpp
void ProcessLayer(Layer* layer) {
    ERROR_CONTEXT("ProcessLayer");
    
    // Any errors here will include "ProcessLayer" in context
    ApplyFilters(layer);
}

void ApplyFilters(Layer* layer) {
    ERROR_CONTEXT("ApplyFilters");
    
    // Context is now: "ProcessLayer > ApplyFilters"
    for (auto& filter : filters) {
        filter.Apply(layer);
    }
}
```

### Nested Context

```cpp
class ImageProcessor {
public:
    void Process() {
        ERROR_CONTEXT("ImageProcessor::Process");
        
        LoadImage();      // Context: ImageProcessor::Process > LoadImage
        Transform();      // Context: ImageProcessor::Process > Transform
        SaveImage();      // Context: ImageProcessor::Process > SaveImage
    }
    
private:
    void LoadImage() {
        ERROR_CONTEXT("LoadImage");
        // ...
    }
    
    void Transform() {
        ERROR_CONTEXT("Transform");
        // ...
    }
    
    void SaveImage() {
        ERROR_CONTEXT("SaveImage");
        // ...
    }
};
```

## Best Practices

### 1. Always Check Results

```cpp
// ❌ Bad - ignoring result
auto result = LoadImage("photo.png");
ProcessImage(result.Value());  // Might crash if load failed!

// ✅ Good - check result
auto result = LoadImage("photo.png");
if (result.IsOk()) {
    ProcessImage(result.Value());
} else {
    HandleError(result.Error());
}
```

### 2. Provide Meaningful Context

```cpp
// ❌ Bad - no context
return ErrorCode::IOFileNotFound;

// ✅ Good - with context
return ErrorInfo(ErrorCode::IOFileNotFound, 
                "Could not find brush file: " + brush_name);
```

### 3. Use Appropriate Severity

```cpp
// ❌ Bad - using exception for expected error
if (!FileExists(path)) {
    throw IOException::FileNotFound(path);
}

// ✅ Good - use Result for expected errors
if (!FileExists(path)) {
    return ErrorInfo(ErrorCode::IOFileNotFound, path);
}

// ✅ Good - exception for unexpected errors
if (gpu_memory_corrupted) {
    THROW_EXCEPTION(
        MemoryException::MemoryCorruption("GPU memory corrupted")
    );
}
```

### 4. Don't Catch and Ignore

```cpp
// ❌ Bad - swallowing errors
try {
    CriticalOperation();
} catch (...) {
    // Ignored - bad!
}

// ✅ Good - handle or propagate
try {
    CriticalOperation();
} catch (const GimpException& e) {
    ErrorHandler::GetInstance().ReportError(e.GetErrorInfo());
    return false;  // Indicate failure to caller
}
```

### 5. Initialize Early, Shutdown Late

```cpp
int main(int argc, char* argv[]) {
    // ✅ Initialize first thing
    ErrorHandler::GetInstance().Initialize();
    
    try {
        RunApplication(argc, argv);
    } catch (...) {
        // Handle
    }
    
    // ✅ Shutdown last thing
    ErrorHandler::GetInstance().Shutdown();
}
```

## Common Patterns

### Pattern 1: File I/O with Result

```cpp
Result<std::string> ReadConfigFile(const std::string& path) {
    ERROR_CONTEXT("ReadConfigFile");
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return ErrorInfo(ErrorCode::IOFileNotFound, path);
    }
    
    std::string content;
    if (!std::getline(file, content, '\0')) {
        return ErrorInfo(ErrorCode::IOReadError, path);
    }
    
    return content;
}

// Usage
auto result = ReadConfigFile("config.ini");
if (result.IsOk()) {
    ParseConfig(result.Value());
} else {
    // Use default config
    UseDefaultConfig();
}
```

### Pattern 2: Resource Allocation with Exceptions

```cpp
class Texture {
public:
    Texture(int width, int height) {
        ERROR_CONTEXT("Texture::Texture");
        
        data_ = new(std::nothrow) uint8_t[width * height * 4];
        if (!data_) {
            THROW_EXCEPTION(
                MemoryException::AllocationFailed(
                    "Texture size: " + std::to_string(width * height * 4)
                )
            );
        }
        
        width_ = width;
        height_ = height;
    }
    
    ~Texture() {
        delete[] data_;
    }
    
private:
    uint8_t* data_;
    int width_, height_;
};
```

### Pattern 3: Validation with Early Return

```cpp
Result<void> ValidateImage(const Image& img) {
    if (img.GetWidth() <= 0) {
        return Err(ErrorCode::InvalidArgumentOutOfRange, 
                  "Width must be positive");
    }
    
    if (img.GetHeight() <= 0) {
        return Err(ErrorCode::InvalidArgumentOutOfRange, 
                  "Height must be positive");
    }
    
    if (img.GetData() == nullptr) {
        return Err(ErrorCode::InvalidArgumentNull, 
                  "Image data is null");
    }
    
    return Ok();
}
```

### Pattern 4: Batch Operations

```cpp
std::vector<ErrorInfo> ProcessBatch(const std::vector<Image>& images) {
    std::vector<ErrorInfo> errors;
    
    for (const auto& img : images) {
        auto result = ProcessImage(img);
        if (!result.IsOk()) {
            errors.push_back(result.Error());
            ErrorHandler::GetInstance().ReportError(result.Error());
        }
    }
    
    return errors;  // Return all errors that occurred
}
```

### Pattern 5: Callback-Based Error Notification

```cpp
void SetupUIErrorHandling() {
    // Show errors in status bar
    ErrorHandler::GetInstance().RegisterCallback(
        [](const ErrorInfo& error) {
            if (error.GetSeverity() >= ErrorSeverity::Recoverable) {
                StatusBar::ShowError(error.GetMessage());
            }
        }
    );
    
    // Log all errors
    ErrorHandler::GetInstance().RegisterCallback(
        [](const ErrorInfo& error) {
            Logger::Log(error.ToString());
        }
    );
}
```

## Testing Error Handling

### Testing Result<T>

```cpp
TEST_CASE("Error handling - Result") {
    // Test success case
    auto result = Divide(10, 2);
    REQUIRE(result.IsOk());
    REQUIRE(result.Value() == 5);
    
    // Test error case
    result = Divide(10, 0);
    REQUIRE(result.IsError());
    REQUIRE(result.Error().GetCode() == ErrorCode::InvalidArgumentOutOfRange);
}
```

### Testing Exceptions

```cpp
TEST_CASE("Error handling - Exceptions") {
    REQUIRE_THROWS_AS(
        LoadBrush(""),
        InvalidArgumentException
    );
    
    try {
        LoadBrush("");
        FAIL("Should have thrown");
    } catch (const InvalidArgumentException& e) {
        REQUIRE(e.GetCode() == ErrorCode::InvalidArgumentNull);
    }
}
```

### Testing Recovery

```cpp
TEST_CASE("Error recovery") {
    bool recovery_called = false;
    
    ErrorHandler::GetInstance().RegisterRecoveryHandler(
        ErrorCategory::IO,
        [&](const ErrorInfo& error) {
            recovery_called = true;
            return true;
        }
    );
    
    ErrorHandler::GetInstance().ReportError(
        ErrorCode::IOFileNotFound, "test.txt"
    );
    
    REQUIRE(recovery_called);
}
```

## Summary Checklist

When implementing error handling in your code:

- [ ] Choose appropriate error strategy (Result vs Exception)
- [ ] Provide meaningful error context
- [ ] Use correct error codes and categories
- [ ] Add ERROR_CONTEXT to functions
- [ ] Register recovery handlers where appropriate
- [ ] Initialize ErrorHandler in main()
- [ ] Handle fatal errors appropriately
- [ ] Test both success and error paths
- [ ] Document expected errors in function comments
- [ ] Don't catch and ignore exceptions

For more details on the architecture, see [ERROR_HANDLING_ARCHITECTURE.md](ERROR_HANDLING_ARCHITECTURE.md).
