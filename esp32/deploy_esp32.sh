#!/bin/zsh

# exit when any command fails
set -e

echo "Setting up ESP-IDF terminal environment..."
source ~/.espressif/v5.5.3/esp-idf/export.sh

cd "$(dirname "$0")"

echo "Terminating any existing serial monitor sessions..."
pkill -f idf_monitor || true

# Small delay to ensure port is released
sleep 1

echo "Building and Flashing ESP32 Emulator Firmware..."
idf.py -p /dev/cu.usbserial-0001 flash monitor
