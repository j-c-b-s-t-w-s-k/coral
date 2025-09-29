#!/bin/bash
# Quick fix for Mac autotools PKG_CHECK_MODULES error

echo "🔧 Fixing Mac autotools configuration..."

# Install pkg-config if missing
brew install pkg-config

# Set up proper paths for both Intel and Apple Silicon Macs
if [ -d "/opt/homebrew/share/aclocal" ]; then
    # Apple Silicon Mac (M1/M2/M3)
    ACLOCAL_DIR="/opt/homebrew/share/aclocal"
    PKG_CONFIG_DIR="/opt/homebrew/lib/pkgconfig"
    LIB_DIR="/opt/homebrew/lib"
    INCLUDE_DIR="/opt/homebrew/include"
elif [ -d "/usr/local/share/aclocal" ]; then
    # Intel Mac
    ACLOCAL_DIR="/usr/local/share/aclocal"
    PKG_CONFIG_DIR="/usr/local/lib/pkgconfig"
    LIB_DIR="/usr/local/lib"
    INCLUDE_DIR="/usr/local/include"
else
    echo "❌ Cannot find Homebrew installation"
    exit 1
fi

echo "Using: $ACLOCAL_DIR"

# Export environment variables
export ACLOCAL_PATH="$ACLOCAL_DIR:$ACLOCAL_PATH"
export PKG_CONFIG_PATH="$PKG_CONFIG_DIR:$PKG_CONFIG_PATH"
export LDFLAGS="-L$LIB_DIR $LDFLAGS"
export CPPFLAGS="-I$INCLUDE_DIR $CPPFLAGS"

# Clean previous attempts
rm -f configure
rm -rf build-aux/m4
rm -f aclocal.m4

# Run aclocal manually with correct paths
echo "Running aclocal with proper macro paths..."
aclocal -I build-aux/m4 --install

# Then run autogen
echo "Running autogen..."
./autogen.sh

echo "✅ Autotools should now be fixed!"
echo "Next, run: ./configure --with-gui=qt5"