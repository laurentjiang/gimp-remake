# GIMP Remake

[![CI](https://github.com/laurentjiang/gimp-remake/actions/workflows/ci.yml/badge.svg)](https://github.com/laurentjiang/gimp-remake/actions/workflows/ci.yml)
[![Documentation](https://github.com/laurentjiang/gimp-remake/actions/workflows/docs.yml/badge.svg)](https://github.com/laurentjiang/gimp-remake/actions/workflows/docs.yml)
[![API Docs](https://img.shields.io/badge/docs-API%20Reference-blue)](https://laurentjiang.github.io/gimp-remake/)
[![License](https://img.shields.io/badge/license-GPL--3.0-green)](LICENSE)

A modern C++20/Qt6/Skia reimplementation of GIMP (GNU Image Manipulation Program).

## Documentation

- **[API Reference](https://laurentjiang.github.io/gimp-remake/)** - Auto-generated Doxygen documentation
- **[Error Handling Guide](docs/ERROR_HANDLING_GUIDE.md)** - How to use the error handling system
- **[Error Handling Architecture](docs/ERROR_HANDLING_ARCHITECTURE.md)** - Design decisions
- **[Quick Reference](docs/ERROR_HANDLING_QUICK_REFERENCE.md)** - Error codes cheat sheet

## Building

### Prerequisites

- **CMake** 3.21+
- **Ninja** build system
- **C++20 compiler**: MSVC 2022, GCC 12+, or Clang 15+
- **vcpkg** (dependencies installed automatically on first build)

### Windows

```powershell
# Clone the repository
git clone https://github.com/laurentjiang/gimp-remake.git
cd gimp-remake

# Build (Debug by default)
.\scripts\build.ps1

# Build Release
.\scripts\build.ps1 -Config Release

# Clean build (preserves vcpkg cache)
.\scripts\build.ps1 -Clean

# Run the application
.\scripts\run.ps1
# or
.\build\gimp-remake.exe

# Run Release version
.\scripts\run.ps1 -Config Release
```

### Running Tests

```powershell
ctest --test-dir build --output-on-failure
```

### Code Quality

```powershell
# Format code (requires clang-format / LLVM 18+)
.\scripts\run-format.ps1

# Run static analysis (requires clang-tidy / LLVM 18+)
.\scripts\run-lint.ps1
```

## Project Structure

```
gimp-remake/
├── include/           # Header files
│   ├── core/          # Core data structures (Layer, Document, Command)
│   ├── error_handling/# Error codes, Result<T>, exceptions
│   ├── io/            # File I/O (images, project files)
│   ├── render/        # Skia-based rendering
│   └── ui/            # Qt6 widgets and panels
├── src/               # Implementation files
│   ├── error_handling/
│   ├── io/
│   └── ui/
├── scripts/           # Build and development scripts
│   ├── build.ps1      # Build the project
│   ├── run.ps1        # Run the application
│   ├── run-format.ps1 # Format code with clang-format
│   └── run-lint.ps1   # Static analysis with clang-tidy
├── tests/             # Unit tests (Catch2)
├── docs/              # Documentation
├── resources/         # Icons, stylesheets
└── Doxyfile           # Doxygen configuration
```
