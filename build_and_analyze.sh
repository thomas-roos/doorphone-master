#!/bin/bash

# Build script with output capture and analysis
BUILD_LOG="build_output.txt"
BUILD_DIR="project/realtek_amebapro2_webrtc_application/GCC-RELEASE/build"

echo "Starting build with output capture..."

# Navigate to build directory
cd "$BUILD_DIR" || exit 1

# Run build and capture all output
cmake --build . --target flash --parallel $(nproc) > "$BUILD_LOG" 2>&1
BUILD_EXIT_CODE=$?

# Analyze build output
echo "Build Analysis:"
echo "==============="
echo "Exit code: $BUILD_EXIT_CODE"

if [ $BUILD_EXIT_CODE -eq 0 ]; then
    echo "Status: SUCCESS"
    
    # Show key metrics
    echo "Generated files:"
    ls -lh *.bin 2>/dev/null || echo "No .bin files found"
    
    # Show summary stats from log
    WARNINGS=$(grep -c "warning:" "$BUILD_LOG" 2>/dev/null || echo "0")
    ERRORS=$(grep -c "error:" "$BUILD_LOG" 2>/dev/null || echo "0")
    
    echo "Warnings: $WARNINGS"
    echo "Errors: $ERRORS"
    
    # Show last few lines for completion confirmation
    echo "Build completion:"
    tail -5 "$BUILD_LOG"
else
    echo "Status: FAILED"
    echo "Last 20 lines of build output:"
    tail -20 "$BUILD_LOG"
fi

echo "Full build log saved to: $BUILD_LOG"
