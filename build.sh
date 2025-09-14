#!/bin/bash
set -e  # Exit on any error

# -------------------------------
# Configuration
# -------------------------------
BUILD_DIR="build"
RUN_CLANG_FORMAT=false   # set true to auto-format
RUN_CLANG_TIDY=false    # set true to auto-run clang-tidy
CLEAN_BUILD=false       # set true to delete old build folder

# -------------------------------
# Parse arguments
# -------------------------------
for arg in "$@"; do
    case $arg in
        --format)
            RUN_CLANG_FORMAT=true
            shift
            ;;
        --tidy)
            RUN_CLANG_TIDY=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: ./build.sh [--clean] [--format] [--tidy]"
            exit 1
            ;;
    esac
done

# Detect number of CPU cores
NUM_CORES=$(sysctl -n hw.ncpu)
echo "Detected $NUM_CORES CPU cores"

# -------------------------------
# Optional: clean build
# -------------------------------
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build folder..."
    rm -rf "$BUILD_DIR"
fi

# -------------------------------
# Create build directory
# -------------------------------
mkdir -p "$BUILD_DIR"

# -------------------------------
# Configure CMake
# -------------------------------
echo "Configuring CMake..."
cmake -S . -B "$BUILD_DIR"

# -------------------------------
# Build project
# -------------------------------
echo "Building project..."
cmake --build "$BUILD_DIR" -j"$NUM_CORES"

# -------------------------------
# Run clang-format (optional)
# -------------------------------
if [ "$RUN_CLANG_FORMAT" = true ]; then
    if command -v clang-format >/dev/null 2>&1; then
        echo "Running clang-format..."
        find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    else
        echo "clang-format not found. Install via brew: brew install clang-format"
    fi
fi

# -------------------------------
# Run clang-tidy (optional)
# -------------------------------
if [ "$RUN_CLANG_TIDY" = true ]; then
    echo "Running clang-tidy..."
    find src/ -name "*.cpp" | xargs /opt/homebrew/opt/llvm/bin/clang-tidy -p "$BUILD_DIR" -export-fixes=fixes.yaml
fi

echo "Build script finished successfully!"
