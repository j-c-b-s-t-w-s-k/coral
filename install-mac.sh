#!/bin/bash
# Coral Cryptocurrency Mac Installation Script
# This script builds and installs Coral on macOS

set -e

echo "🪸 Coral Cryptocurrency Mac Installer"
echo "====================================="

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is designed for macOS only"
    echo "   For Linux, use the standard build process"
    exit 1
fi

echo "📋 Step 1: Installing Dependencies"
echo "-----------------------------------"

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install required dependencies
echo "Installing build dependencies..."
brew install automake libtool boost pkg-config libevent openssl cmake

# Install optional dependencies for full functionality
echo "Installing optional dependencies..."
brew install qt@5 qrencode miniupnpc libnatpmp zeromq berkeley-db@4

echo "📦 Step 2: Building RandomX Library"
echo "-----------------------------------"

# Build RandomX from included source
cd RandomX-lib
if [ ! -d "build" ]; then
    mkdir build
fi
cd build

echo "Configuring RandomX for Mac..."
cmake -DARCH=native -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 ..

echo "Building RandomX..."
make -j$(sysctl -n hw.physicalcpu)

echo "Installing RandomX system-wide..."
sudo make install

# Update library paths
sudo ldconfig 2>/dev/null || true

cd ../..

echo "🔧 Step 3: Building Coral"
echo "-------------------------"

# Fix autotools issues
echo "Preparing build system..."
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
export LDFLAGS="-L/usr/local/lib -L/opt/homebrew/lib $LDFLAGS"
export CPPFLAGS="-I/usr/local/include -I/opt/homebrew/include $CPPFLAGS"

# Fix pkg-config macro path for both Intel and Apple Silicon Macs
if [ -d "/opt/homebrew/share/aclocal" ]; then
    # Apple Silicon Mac
    export ACLOCAL_PATH="/opt/homebrew/share/aclocal:$ACLOCAL_PATH"
elif [ -d "/usr/local/share/aclocal" ]; then
    # Intel Mac
    export ACLOCAL_PATH="/usr/local/share/aclocal:$ACLOCAL_PATH"
fi

# Manually run aclocal with proper paths before autogen
echo "Setting up autotools macros..."
aclocal -I build-aux/m4

# Generate configure script
./autogen.sh

# Configure for Mac build
echo "Configuring Coral for Mac..."
./configure \
    --with-gui=qt5 \
    --enable-wallet \
    --with-boost-libdir=/usr/local/lib \
    --with-berkeley-db=/usr/local/opt/berkeley-db@4

# Build Coral
echo "Building Coral (this may take 10-30 minutes)..."
make -j$(sysctl -n hw.physicalcpu)

echo "🎯 Step 4: Installation Complete!"
echo "================================="

echo "✅ Coral has been successfully built!"
echo ""
echo "📍 Executables location:"
echo "   Daemon:    ./src/corald"
echo "   CLI:       ./src/coral-cli"
echo "   GUI:       ./src/qt/coral-qt (if built with Qt)"
echo ""
echo "🚀 Quick Start:"
echo "   1. Start daemon: ./src/corald &"
echo "   2. Generate address: ./src/coral-cli getnewaddress"
echo "   3. Start mining: ./src/coral-cli generatetoaddress 1 <address>"
echo ""
echo "📖 For more info: cat README.md"
echo ""
echo "🏆 Welcome to the Coral mining revolution!"