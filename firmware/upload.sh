#!/bin/bash
set -e

PORT="/dev/ttyUSB0"
FQBN="esp32:esp32:esp32"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

cd "$PROJECT_DIR"

echo "==> 1. Releasing serial port ($PORT)..."
sudo fuser -k "$PORT" 2>/dev/null || true

echo "==> 2. Compiling firmware to ./build..."
arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$BUILD_DIR" .

echo "==> 3. Uploading firmware to ESP32-WROOM-32..."
arduino-cli upload \
  -p "$PORT" \
  --fqbn "$FQBN" \
  --build-path "$BUILD_DIR" .

if [[ "$1" == "--fs" ]]; then
  echo "==> 4. Packaging LittleFS image (1441792 bytes)..."
  rm -f littlefs.bin
  /home/cobalt/.arduino15/packages/esp32/tools/mklittlefs/4.0.2-db0513a/mklittlefs \
    -c "$PROJECT_DIR/data" \
    -p 256 -b 4096 -s 1441792 littlefs.bin

  echo "==> 5. Flashing LittleFS to 0x290000..."
  ESPTOOL="/home/cobalt/.arduino15/packages/esp32/tools/esptool_py/5.3.1/esptool"
  [ ! -f "$ESPTOOL" ] && ESPTOOL="esptool.py"

  "$ESPTOOL" --chip esp32 --port "$PORT" --baud 921600 write-flash 0x290000 littlefs.bin
else
  echo "==> Skipping LittleFS (use './upload.sh --fs' to flash data/)."
fi

echo "==> Done!"
