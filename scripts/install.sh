#!/usr/bin/env bash
set -euo pipefail

# Xeris Ledger App Installer
#
# Prerequisites:
#   pip install ledgerwallet
#
# Usage:
#   ./scripts/install.sh [device]
#
# Supported devices: nanos, nanox, nanosp, stax, flex
# Default: stax

DEVICE="${1:-stax}"
APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/app"

echo "=== Xeris Ledger App Installer ==="
echo "Device: $DEVICE"
echo "App directory: $APP_DIR"
echo ""

# Check ledgerctl is installed
if ! command -v ledgerctl &> /dev/null; then
    echo "Error: ledgerctl not found. Install with: pip install ledgerwallet"
    exit 1
fi

# Check the app was built
ELF_PATH="$APP_DIR/bin/app.elf"
if [ ! -f "$ELF_PATH" ]; then
    echo "Error: App not built. Run 'make docker-build' first."
    exit 1
fi

echo "Step 1: Deleting old Xeris app (if installed)..."
ledgerctl delete "Xeris" 2>/dev/null || true

echo "Step 2: Installing Xeris app..."
ledgerctl install "$APP_DIR/bin/app.hex" \
    --appName "Xeris" \
    --appVersion "1.0.0" \
    --icon "$APP_DIR/glyphs/icon_xeris_nanox.gif" \
    --dataSize 0

echo ""
echo "Installation complete!"
echo "The Xeris app should now appear on your $DEVICE device."
echo ""
echo "To use it:"
echo "  1. Navigate to the Xeris app on your device"
echo "  2. Open the app"
echo "  3. Connect your wallet software to the device"
