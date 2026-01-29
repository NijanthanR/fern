#!/bin/bash
# Install Fern extension for Zed editor

set -e

ZED_EXTENSIONS_DIR="$HOME/Library/Application Support/Zed/extensions/installed"
EXTENSION_NAME="fern"
EXTENSION_DIR="$(cd "$(dirname "$0")/zed-fern" && pwd)"

echo "Installing Fern extension to Zed..."

# Create extensions directory if it doesn't exist
mkdir -p "$ZED_EXTENSIONS_DIR"

# Remove existing symlink/directory if present
if [ -e "$ZED_EXTENSIONS_DIR/$EXTENSION_NAME" ]; then
    echo "Removing existing $EXTENSION_NAME extension..."
    rm -rf "$ZED_EXTENSIONS_DIR/$EXTENSION_NAME"
fi

# Create symlink to development extension
echo "Symlinking $EXTENSION_DIR to Zed extensions..."
ln -s "$EXTENSION_DIR" "$ZED_EXTENSIONS_DIR/$EXTENSION_NAME"

echo "✓ Fern extension installed successfully!"
echo ""
echo "The extension is symlinked for development."
echo "Changes to the extension will be reflected immediately."
echo ""
echo "To reload the extension in Zed:"
echo "  - Open Command Palette (Cmd+Shift+P)"
echo "  - Run: 'zed: reload extensions'"
echo ""
echo "To uninstall:"
echo "  rm -rf '$ZED_EXTENSIONS_DIR/$EXTENSION_NAME'"
