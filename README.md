# GIMP Remake

[![CI](https://github.com/laurentjiang/gimp-remake/actions/workflows/ci.yml/badge.svg)](https://github.com/laurentjiang/gimp-remake/actions/workflows/ci.yml)
[![Documentation](https://github.com/laurentjiang/gimp-remake/actions/workflows/docs.yml/badge.svg)](https://github.com/laurentjiang/gimp-remake/actions/workflows/docs.yml)
[![API Docs](https://img.shields.io/badge/docs-API%20Reference-blue)](https://laurentjiang.github.io/gimp-remake/)

A modern C++20/Qt6/Skia reimplementation of GIMP (GNU Image Manipulation Program).

## Documentation

- **[API Reference](https://laurentjiang.github.io/gimp-remake/)** - Auto-generated Doxygen documentation
- **[Error Handling Guide](docs/ERROR_HANDLING_GUIDE.md)** - How to use the error handling system

## Prerequisites

### Required Tools

| Tool | Version | Installation |
|------|---------|--------------|
| **Visual Studio 2026 Build Tools** | 14.50+ | [Download](https://visualstudio.microsoft.com/downloads/) |
| **CMake** | 3.26+ | Included with VS or `winget install Kitware.CMake` |
| **Ninja** | 1.11+ | Included with VS or `winget install Ninja-build.Ninja` |
| **vcpkg** | cc73782a88db48af17f8bfb8328d4cab3d4c246f | See [vcpkg setup](#vcpkg-setup) below |
| **clang-format / clang-tidy** | 18+ | Included with VS 2026 Build Tools |

### vcpkg Setup

```powershell
# Clone vcpkg using "cc73782a88db48af17f8bfb8328d4cab3d4c246f" baseline (one-time setup)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Set environment variable (add to your profile)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
$env:VCPKG_ROOT = "C:\vcpkg"
```

Dependencies (Qt6, Skia, OpenCV, spdlog, nlohmann-json) are installed automatically during the first build via vcpkg manifest mode.

## Building

### Quick Start

```powershell
# Clone the repository
git clone https://github.com/laurentjiang/gimp-remake.git
cd gimp-remake

# Build (Debug by default)
.\scripts\build.ps1

# Run the application
.\scripts\run.ps1
```

### Build Options

```powershell
# Debug build (default)
.\scripts\build.ps1

# Release build
.\scripts\build.ps1 -Config Release

# Clean build (preserves vcpkg cache for faster rebuilds)
.\scripts\build.ps1 -Clean

# Build with coverage instrumentation
.\scripts\build.ps1 -Coverage
```

### Running

```powershell
# Run Debug build
.\scripts\run.ps1

# Run Release build
.\scripts\run.ps1 -Config Release

# Or run directly
.\build\gimp-remake.exe
```

## Testing

```powershell
# Run all tests with coverage report
.\scripts\run-tests.ps1

# Run tests without coverage
.\scripts\run-tests.ps1 -NoCoverage

# Run with verbose output
.\scripts\run-tests.ps1 -Verbose

# Filter specific tests
.\scripts\run-tests.ps1 -Filter "[brush]"

# Or use ctest directly
ctest --test-dir build --output-on-failure
```

## Code Quality

```powershell
# Format all code (clang-format)
.\scripts\run-format.ps1

# Run static analysis (clang-tidy)
.\scripts\run-lint.ps1
```

## Project Structure

```
gimp-remake/
├── include/               # Header files (mirrors src/ structure)
│   ├── core/              # Domain: Document, Layer, Tool, Command, Filter
│   │   ├── commands/      # DrawCommand, AddLayerCommand
│   │   ├── filters/       # BlurFilter, SharpenFilter
│   │   └── tools/         # PencilTool, BrushTool, FillTool, etc.
│   ├── error_handling/    # Error codes, Result<T>
│   ├── history/           # HistoryStack, HistoryManager
│   ├── io/                # IOManager (image loading/saving)
│   ├── render/            # SkiaRenderer, SkiaCompositor
│   └── ui/                # Qt widgets: MainWindow, panels, theme
├── src/                   # Implementation files
├── tests/
│   ├── unit/              # Pure logic tests
│   ├── integration/       # File I/O and rendering tests
│   └── img/               # Test image fixtures
├── scripts/               # Build and development scripts
│   ├── build.ps1          # CMake configure + build
│   ├── run.ps1            # Launch application
│   ├── run-tests.ps1      # Run tests with optional coverage
│   ├── run-format.ps1     # Format code with clang-format
│   └── run-lint.ps1       # Static analysis with clang-tidy
├── resources/             # Qt resources (icons, qrc)
├── docs/                  # Additional documentation
├── .github/workflows/     # CI pipeline
├── CMakeLists.txt         # Build configuration
├── vcpkg.json             # Dependency manifest
├── .clang-format          # Code formatting rules
└── .clang-tidy            # Static analysis configuration
```
