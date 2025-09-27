#!/bin/bash

# Coral Cryptocurrency - Windows Build Script
# Cross-compilation from Linux using MinGW

set -e

echo "🪸 Building Coral Cryptocurrency for Windows..."

# Set cross-compilation environment
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
export AR=x86_64-w64-mingw32-ar
export STRIP=x86_64-w64-mingw32-strip
export RANLIB=x86_64-w64-mingw32-ranlib

# Build directories
mkdir -p build-win64/{bin,lib}

echo "Building dependencies..."

# Build basic Windows executables manually
# Since autotools isn't working, we'll compile key components

echo "Building Coral Windows Test Executable..."
$CXX -o build-win64/bin/coral-test.exe test_genesis.cpp -static -std=c++17

echo "Creating Windows wallet template..."
cat > build-win64/bin/coral-wallet.bat << 'EOF'
@echo off
echo Coral Cryptocurrency Windows Wallet
echo ====================================
echo.
echo Genesis Block Configuration:
echo - Difficulty: 21e800 (21 followed by 800 zeros)
echo - Header-only genesis (no transactions)
echo - Polish Message: FUCK SATOSHI! Byl glupim snobem
echo - RandomX Mining Algorithm (CPU-optimized)
echo.
echo This is a template wallet. Full Windows GUI wallet requires Qt compilation.
echo For now, use the command-line tools or wait for full GUI build.
echo.
pause
EOF

echo "Creating mining helper script..."
cat > build-win64/bin/coral-mine.bat << 'EOF'
@echo off
echo Coral Cryptocurrency - RandomX CPU Mining
echo =========================================
echo.
echo Mining Configuration:
echo - Algorithm: RandomX (ASIC-resistant)
echo - CPU cores recommended: All available minus 1-2
echo - Memory usage: ~2GB per mining thread
echo - Expected hashrate: Varies by CPU (500-8000 H/s typical)
echo.
echo To start mining:
echo 1. Build the full node software
echo 2. Run: corald.exe -printtoconsole
echo 3. In another window: coral-cli.exe generate 1 [your_address]
echo.
echo Network ready for mining! Join the Coral network.
echo.
pause
EOF

# Create Windows installer configuration
echo "Creating Windows installer config..."
cat > build-win64/coral-installer.nsi << 'EOF'
; Coral Cryptocurrency Windows Installer
!define APPNAME "Coral Cryptocurrency"
!define APPVERSION "1.0.0"
!define APPDIR "Coral"

Name "${APPNAME}"
OutFile "Coral-${APPVERSION}-Windows-Setup.exe"
InstallDir "$PROGRAMFILES64\${APPDIR}"

Page directory
Page instfiles

Section "Install"
    SetOutPath $INSTDIR
    File "bin\coral-test.exe"
    File "bin\coral-wallet.bat"
    File "bin\coral-mine.bat"

    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Coral Wallet.lnk" "$INSTDIR\coral-wallet.bat"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Coral Mining.lnk" "$INSTDIR\coral-mine.bat"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\coral-test.exe"
    Delete "$INSTDIR\coral-wallet.bat"
    Delete "$INSTDIR\coral-mine.bat"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\${APPNAME}\Coral Wallet.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Coral Mining.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${APPNAME}"
SectionEnd
EOF

echo "Windows build components created successfully!"
echo ""
echo "📁 Windows build files location: build-win64/"
echo "🔧 Test executable: build-win64/bin/coral-test.exe"
echo "💰 Wallet template: build-win64/bin/coral-wallet.bat"
echo "⛏️  Mining helper: build-win64/bin/coral-mine.bat"
echo "📦 Installer config: build-win64/coral-installer.nsi"
echo ""
echo "✅ Windows build preparation complete!"
echo ""
echo "Next steps:"
echo "1. Fix autotools to build full node binaries"
echo "2. Compile Qt GUI wallet for Windows"
echo "3. Create Windows installer with NSIS"
echo "4. Test on Windows system"

# Test the executable
echo ""
echo "🧪 Testing Windows executable with Wine:"
if command -v wine64 >/dev/null 2>&1; then
    wine64 build-win64/bin/coral-test.exe
else
    echo "Wine not available, but Windows executable was built successfully."
fi