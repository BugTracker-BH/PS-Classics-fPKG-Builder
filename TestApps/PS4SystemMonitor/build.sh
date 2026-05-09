#!/bin/bash
# Build script for PS4 System Monitor
# Requires: OpenOrbis SDK

set -e

echo "=== Elite Kinyamon's PS4 System Monitor ==="
echo ""

if [ -z "$OO_PS4_TOOLCHAIN" ]; then
    echo "ERROR: OO_PS4_TOOLCHAIN environment variable not set!"
    echo "Set it to your OpenOrbis SDK path, e.g.:"
    echo "  export OO_PS4_TOOLCHAIN=/opt/OpenOrbis"
    exit 1
fi

echo "[1/3] Compiling source..."
make clean
make all

echo "[2/3] Verifying eboot.bin..."
if [ -f "app/eboot.bin" ]; then
    echo "  OK - eboot.bin generated ($(stat -c%s app/eboot.bin 2>/dev/null || stat -f%z app/eboot.bin) bytes)"
else
    echo "  FAIL - eboot.bin not found!"
    exit 1
fi

echo "[3/3] Ready for PKG creation!"
echo ""
echo "Next steps:"
echo "  1. Replace app/sce_sys/icon0.png with a real 512x512 PNG"
echo "  2. Replace app/sce_sys/pic0.png with a real 1920x1080 PNG"
echo "  3. Open PS Classics fPKG Builder -> PS4 App tab"
echo "  4. Set App Folder to: $(pwd)/app/"
echo "  5. Title: Elite Kinyamon's System Monitor"
echo "  6. Title ID: EKMS00001"
echo "  7. Content ID: UP0001-EKMS00001_00-SYSTEMMONITOR001"
echo "  8. Click Build fPKG!"
echo ""
echo "=== Build complete ==="
