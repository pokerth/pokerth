#!/bin/bash

# Set environment variables from Dockerfile
ROOT=${ROOT:-/opt/pokerth-windows}
QT_VERSION=${QT_VERSION:-6.9.3}

# Set Qt paths
export QT_WINDOWS_DIR=${ROOT}/Qt/${QT_VERSION}/mingw_64
export QT_HOST_PATH=${ROOT}/Qt/${QT_VERSION}/gcc_64
export CMAKE_PREFIX_PATH=${QT_WINDOWS_DIR}
export Qt6_DIR=${QT_WINDOWS_DIR}/lib/cmake/Qt6

# Set vcpkg paths
export VCPKG_ROOT=${ROOT}/vcpkg
export CMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
export VCPKG_TARGET_TRIPLET=x64-mingw-static
# Set MinGW path
export MINGW_DIR=/usr/x86_64-w64-mingw32

# Add qt-cmake to PATH
export PATH=${QT_HOST_PATH}/bin:${PATH}

cd ${ROOT}/pokerth

# Set the build directory
BUILD_DIR="./build"
DEPLOY_DIR="${BUILD_DIR}/deploy"

# Remove old build directory to start fresh
# rm -rf $BUILD_DIR

# Create the build directory
mkdir -p $BUILD_DIR

echo "Building with:"
echo "  Qt Windows: ${QT_WINDOWS_DIR}"
echo "  Qt Host: ${QT_HOST_PATH}"
echo "  Qt6_DIR: ${Qt6_DIR}"
echo "  vcpkg: ${VCPKG_ROOT}"
echo "  Toolchain: ${CMAKE_TOOLCHAIN_FILE}"



# Run CMake to configure the project for Windows cross-compilation
qt-cmake -S . -B $BUILD_DIR \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} \
    -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} \
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} \
    -DQt6_DIR=${Qt6_DIR} \
    -DQT_HOST_PATH=${QT_HOST_PATH} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_FLAGS="-fpermissive -Wno-error" \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_FIND_ROOT_PATH=${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET} \
    -DQT_NO_DEPLOY=ON \
    -DQT_DEPLOY_SUPPORT=OFF

# Build the project
cmake --build $BUILD_DIR  --target ${TARGET} --parallel $(nproc) 2>&1

if [ $? -ne 0 ]; then
    echo "Build failed! Check build.log for details."
    exit 1
fi

echo "Build successful! Collecting DLLs and data files..."

# Create deployment directory
mkdir -p $DEPLOY_DIR

# Collect all required DLLs for deployment
echo "Copying executable..."
if [ -d "$BUILD_DIR/bin" ]; then
    cp $BUILD_DIR/bin/*.exe $DEPLOY_DIR/ 2>/dev/null || echo "  Warning: No executables found in bin/"
fi

echo "Copying Qt DLLs..."
cp ${QT_WINDOWS_DIR}/bin/Qt6Core.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Gui.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Widgets.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Network.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Sql.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Xml.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6WebSockets.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Multimedia.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6MultimediaWidgets.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Qml.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Quick.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6QuickControls2.dll $DEPLOY_DIR/ 2>/dev/null || true
cp ${QT_WINDOWS_DIR}/bin/Qt6Svg.dll $DEPLOY_DIR/ 2>/dev/null || true

echo "Copying Qt plugins..."
mkdir -p $DEPLOY_DIR/plugins/platforms
mkdir -p $DEPLOY_DIR/plugins/styles
mkdir -p $DEPLOY_DIR/plugins/imageformats
mkdir -p $DEPLOY_DIR/plugins/sqldrivers
mkdir -p $DEPLOY_DIR/plugins/tls

cp -r ${QT_WINDOWS_DIR}/plugins/platforms/*.dll $DEPLOY_DIR/plugins/platforms/ 2>/dev/null || true
cp -r ${QT_WINDOWS_DIR}/plugins/styles/*.dll $DEPLOY_DIR/plugins/styles/ 2>/dev/null || true
cp -r ${QT_WINDOWS_DIR}/plugins/imageformats/*.dll $DEPLOY_DIR/plugins/imageformats/ 2>/dev/null || true
cp -r ${QT_WINDOWS_DIR}/plugins/sqldrivers/*.dll $DEPLOY_DIR/plugins/sqldrivers/ 2>/dev/null || true
cp -r ${QT_WINDOWS_DIR}/plugins/tls/*.dll $DEPLOY_DIR/plugins/tls/ 2>/dev/null || true


echo "Copying MinGW runtime DLLs from vcpkg..."
# vcpkg installiert MinGW-Runtime-DLLs im bin-Verzeichnis
VCPKG_BIN_DIR=${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/bin

# Kopiere MinGW Runtime DLLs
if [ -d "$VCPKG_BIN_DIR" ]; then
    echo "  Copying from vcpkg: $VCPKG_BIN_DIR"
    cp ${VCPKG_BIN_DIR}/*.dll $DEPLOY_DIR/ 2>/dev/null || true
fi

# Fallback: Suche die DLLs im System-MinGW
if [ ! -f "$DEPLOY_DIR/libgcc_s_seh-1.dll" ]; then
    echo "  Searching for MinGW runtime DLLs in system paths..."
    find /usr/lib/gcc/x86_64-w64-mingw32 -name "libgcc_s_seh-1.dll" -exec cp {} $DEPLOY_DIR/ \; 2>/dev/null || true
    find /usr/lib/gcc/x86_64-w64-mingw32 -name "libstdc++-6.dll" -exec cp {} $DEPLOY_DIR/ \; 2>/dev/null || true
    find /usr/x86_64-w64-mingw32 -name "libwinpthread-1.dll" -exec cp {} $DEPLOY_DIR/ \; 2>/dev/null || true
    
    # Alternative Pfade
    find /usr/lib/gcc-cross -name "libgcc_s_seh-1.dll" -exec cp {} $DEPLOY_DIR/ \; 2>/dev/null || true
    find /usr/lib/gcc-cross -name "libstdc++-6.dll" -exec cp {} $DEPLOY_DIR/ \; 2>/dev/null || true
fi

# Überprüfe, ob alle DLLs gefunden wurden
echo "  Verifying MinGW runtime DLLs..."
for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    if [ -f "$DEPLOY_DIR/$dll" ]; then
        echo "    ✓ $dll"
    else
        echo "    ✗ $dll (MISSING!)"
    fi
done

# Copy data directory
echo "Copying data directory..."
if [ -d "data" ]; then
    cp -r data $DEPLOY_DIR/
    echo "  Data directory copied successfully"
else
    echo "  Warning: data directory not found in ${ROOT}/pokerth"
fi

# Create qt.conf to help Qt find plugins
echo "Creating qt.conf..."
cat > $DEPLOY_DIR/qt.conf << EOF
[Paths]
Plugins = plugins
EOF

# Create launcher script with proper locale and Qt settings
echo "Creating launcher script..."
cat > $DEPLOY_DIR/pokerth_launcher.bat << 'EOF'
@echo off
chcp 65001
set LANG=en_US.UTF-8
set LC_ALL=en_US.UTF-8
set QT_QPA_PLATFORM=windows
set QT_DEBUG_PLUGINS=0
start "" "%~dp0pokerth_client.exe" %*
EOF

# Create Wine launcher with locale and Qt settings
cat > $DEPLOY_DIR/run_pokerth.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
export QT_QPA_PLATFORM=windows
export WINEDEBUG=-all
wine pokerth_client.exe "$@"
EOF
chmod +x $DEPLOY_DIR/run_pokerth.sh

# Check if qwindows.dll exists in platforms plugin
echo "Verifying Qt platform plugin..."
if [ -f "$DEPLOY_DIR/plugins/platforms/qwindows.dll" ]; then
    echo "  ✓ qwindows.dll found"
else
    echo "  ✗ qwindows.dll MISSING!"
    echo "  Searching for qwindows.dll..."
    find ${QT_WINDOWS_DIR} -name "qwindows.dll" -exec cp {} $DEPLOY_DIR/plugins/platforms/ \; 2>/dev/null || true
fi

# Copy additional Qt dependencies
echo "Copying additional Qt dependencies..."
mkdir -p $DEPLOY_DIR/plugins/generic
cp -r ${QT_WINDOWS_DIR}/plugins/generic/*.dll $DEPLOY_DIR/plugins/generic/ 2>/dev/null || true

# Copy zlib and other common dependencies
if [ -d "$VCPKG_BIN_DIR" ]; then
    cp ${VCPKG_BIN_DIR}/zlib*.dll $DEPLOY_DIR/ 2>/dev/null || true
    cp ${VCPKG_BIN_DIR}/libpng*.dll $DEPLOY_DIR/ 2>/dev/null || true
    cp ${VCPKG_BIN_DIR}/libjpeg*.dll $DEPLOY_DIR/ 2>/dev/null || true
fi

echo ""
echo "======================================"
echo "Build process completed successfully!"
echo "======================================"
echo ""
echo "Deployment package created in: ${DEPLOY_DIR}"
echo ""
echo "Files included:"
ls -lh $DEPLOY_DIR/*.exe 2>/dev/null || echo "  No executables found!"
echo ""
echo "Data files:"
if [ -d "$DEPLOY_DIR/data" ]; then
    echo "  data/ directory included with $(find $DEPLOY_DIR/data -type f | wc -l) files"
else
    echo "  No data directory"
fi
echo ""

# Create Windows Installer with NSIS
echo "======================================"
echo "Creating Windows Installer with NSIS"
echo "======================================"

# Check if NSIS is installed
if ! command -v makensis &> /dev/null; then
    echo "NSIS not found. Installing..."
    apt-get update && apt-get install -y nsis
    
    if [ $? -ne 0 ]; then
        echo "Failed to install NSIS. Installer creation skipped."
        echo "You can install NSIS manually with: apt-get install nsis"
        exit 0
    fi
fi

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Create icon from SVG if ImageMagick is available
if command -v convert &> /dev/null; then
    echo "Converting SVG to ICO..."
    if [ -f "${ROOT}/pokerth/pokerth.svg" ]; then
        convert -background none -density 256 "${ROOT}/pokerth/pokerth.svg" \
                -define icon:auto-resize=256,128,64,48,32,16 \
                "${SCRIPT_DIR}/pokerth.ico" 2>/dev/null || \
        echo "  Warning: Could not convert SVG to ICO"
    elif [ -f "${ROOT}/pokerth/pokerth.png" ]; then
        convert "${ROOT}/pokerth/pokerth.png" -resize 256x256 \
                -define icon:auto-resize=256,128,64,48,32,16 \
                "${SCRIPT_DIR}/pokerth.ico" 2>/dev/null || \
        echo "  Warning: Could not convert PNG to ICO"
    fi
else
    echo "  ImageMagick not found, skipping icon conversion"
    echo "  Install with: apt-get install imagemagick"
fi

# Copy icon to deploy directory if it exists
if [ -f "${SCRIPT_DIR}/pokerth.ico" ]; then
    cp "${SCRIPT_DIR}/pokerth.ico" "${DEPLOY_DIR}/" 2>/dev/null
fi

# Create the installer
echo "Running makensis..."
cd "${SCRIPT_DIR}"

if makensis -NOCD installer.nsi; then
    echo ""
    echo "======================================"
    echo "Installer created successfully!"
    echo "======================================"
    
    # Find the created installer
    INSTALLER=$(find . -name "PokerTH-*-Setup.exe" -type f -printf "%T@ %p\n" | sort -n | tail -1 | cut -d' ' -f2-)
    
    if [ -n "$INSTALLER" ]; then
        INSTALLER_SIZE=$(du -h "$INSTALLER" | cut -f1)
        echo ""
        echo "Installer: ${INSTALLER}"
        echo "Size: ${INSTALLER_SIZE}"
        echo ""
        echo "The installer includes:"
        echo "  ✓ PokerTH Game Client"
        echo "  ✓ All required DLLs and dependencies"
        echo "  ✓ Game data files (graphics, sounds, translations)"
        echo "  ✓ Desktop shortcut"
        echo "  ✓ Start Menu entries"
        echo "  ✓ Uninstaller"
        echo ""
    fi
else
    echo ""
    echo "======================================"
    echo "Installer creation failed!"
    echo "======================================"
    echo "Please check the NSIS output above for errors."
    echo ""
fi

echo ""
echo "To test the deployment package with Wine on Linux:"
echo "  cd ${DEPLOY_DIR}"
echo "  wine pokerth_client.exe"
echo ""
echo "To test the installer with Wine:"
echo "  wine ${SCRIPT_DIR}/PokerTH-*-Setup.exe"
echo ""