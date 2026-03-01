/*
 * Copyright 2026 G.Pimblott
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ESP32Screen.h"
#include "../../src/spectrum/video/VideoBuffer.h"

// We only include TFT_eSPI when compiling on the ESP32
#ifdef ESP_PLATFORM
#include <TFT_eSPI.h>
#else
// Mock for syntax highlighting outside IDF
class TFT_eSPI {
public:
  void init() {}
  void setRotation(int r) {}
  void fillScreen(uint16_t c) {}
  void setAddrWindow(int x, int y, int w, int h) {}
  void pushColors(uint16_t *data, int len) {}
};
#endif

ESP32Screen::ESP32Screen()
    : tft(nullptr), scanlineBuffer(nullptr), flashCounter(0) {
  // 256 pixels wide, each pixel is a 16-bit RGB565 color (2 bytes)
  scanlineBuffer = new uint16_t[SPECTRUM_SCREEN_WIDTH];
}

ESP32Screen::~ESP32Screen() {
  if (tft)
    delete tft;
  if (scanlineBuffer)
    delete[] scanlineBuffer;
}

void ESP32Screen::initPalette() {
  // ZX Spectrum Colors (0-7 Normal, 8-15 Bright)
  // Converted to RGB565 format (5 bits red, 6 bits green, 5 bits blue)
  // Format: (R & 0xF8) << 8 | (G & 0xFC) << 3 | (B >> 3)

  // Normal Colors
  palette[0] = 0x0000; // Black
  palette[1] = 0x0019; // Blue (0, 0, 205)
  palette[2] = 0xCB00; // Red (205, 0, 0)
  palette[3] = 0xCB19; // Magenta (205, 0, 205)
  palette[4] = 0x0660; // Green (0, 205, 0)
  palette[5] = 0x0679; // Cyan (0, 205, 205)
  palette[6] = 0xCE60; // Yellow (205, 205, 0)
  palette[7] = 0xCE79; // White (205, 205, 205)

  // Bright Colors
  palette[8] = 0x0000;  // Black
  palette[9] = 0x001F;  // Bright Blue (0, 0, 255)
  palette[10] = 0xF800; // Bright Red (255, 0, 0)
  palette[11] = 0xF81F; // Bright Magenta (255, 0, 255)
  palette[12] = 0x07E0; // Bright Green (0, 255, 0)
  palette[13] = 0x07FF; // Bright Cyan (0, 255, 255)
  palette[14] = 0xFFE0; // Bright Yellow (255, 255, 0)
  palette[15] = 0xFFFF; // Bright White (255, 255, 255)
}

void ESP32Screen::init(VideoBuffer *buffer) {
  this->videoBuffer = buffer;

  // Initialize color map
  initPalette();

  tft = new TFT_eSPI();
  tft->init();

  // Set landscape Mode (rotation 1 or 3 depending on how screen is mounted)
  tft->setRotation(1);

  // Ensure the byte order is correct for the pushImage API since we are pushing
  // Little-Endian ESP32 memory to Big-Endian SPI
  tft->setSwapBytes(true);

  // Fill the screen with the default border color
  uint8_t borderIdx = videoBuffer->getBorderColor();
  tft->fillScreen(palette[borderIdx]);
}

void ESP32Screen::show() {
  // Nothing required for hardware screen.
}

void ESP32Screen::hide() {
  // Turn off backlight in future?
}

void ESP32Screen::renderCharacterBlock(int x, int y, uint16_t *destBuffer,
                                       bool flashInvert) {
  // Address format: 010YYYyy yyyxxxxx
  // x = 0..31 (character column)
  // y = 0..191 (pixel row)

  uint8_t pixelByte = videoBuffer->getByte(x, y);
  uint8_t attrByte = videoBuffer->getAttribute(x, y);

  // Decode attribute byte: F B P P P I I I
  // F: Flash, B: Bright, P: Paper, I: Ink
  bool flash = (attrByte & 0x80) != 0;
  bool bright = (attrByte & 0x40) != 0;
  uint8_t paper = (attrByte >> 3) & 0x07;
  uint8_t ink = attrByte & 0x07;

  // Apply bright bit offset
  if (bright) {
    paper += 8;
    ink += 8;
  }

  // Determine colors based on flash state
  uint16_t inkColor, paperColor;
  if (flash && flashInvert) {
    inkColor = palette[paper];
    paperColor = palette[ink];
  } else {
    inkColor = palette[ink];
    paperColor = palette[paper];
  }

  // Render 8 pixels (1 bit each)
  // Leftmost pixel is MSB (bit 7)
  int destOffset = x * 8;
  for (int bit = 7; bit >= 0; --bit) {
    bool pixelIsSet = (pixelByte & (1 << bit)) != 0;
    destBuffer[destOffset++] = pixelIsSet ? inkColor : paperColor;
  }
}

void ESP32Screen::update() {
  if (!videoBuffer || !tft)
    return;

  // The ZX Spectrum flashes colors every 16 frames
  flashCounter++;
  bool flashInvert = (flashCounter & 0x10) != 0;

  // The TFT screen is likely 320x240, while spectrum is 256x192.
  // Calculate an offset to center the display.
  // X Offset = (320 - 256) / 2 = 32
  // Y Offset = (240 - 192) / 2 = 24
  int xOffset = 32;
  int yOffset = 24;

  // Update the screen line by line.
  // For ultimate performance on ESP32, TFT_eSPI DMA mode should be used here.
  // However, pushColors handles the windowing efficiently.
  for (int y = 0; y < SPECTRUM_SCREEN_HEIGHT; ++y) {
    // Process each of the 32 character blocks per line
    for (int x = 0; x < VIDEO_WIDTH_CHARS; ++x) {
      renderCharacterBlock(x, y, scanlineBuffer, flashInvert);
    }

    // Define hardware window for this single scanline and push it using
    // pushImage pushImage is safer than setAddrWindow + pushColors because it
    // manages the SPI Chip Select (CS)
    tft->pushImage(xOffset, yOffset + y, SPECTRUM_SCREEN_WIDTH, 1,
                   scanlineBuffer);
  }

  // We would also update the border colors in the xOffset/yOffset regions here
  // based on videoBuffer->getBorderColorAtLine(y).
}

bool ESP32Screen::processEvents() {
  // Embedded system has no OS events to pump.
  // Return true to keep the emulator running.
  return true;
}
