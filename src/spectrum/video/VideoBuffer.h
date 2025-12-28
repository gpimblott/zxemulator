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

#ifndef ZXEMULATOR_VIDEOBUFFER_H
#define ZXEMULATOR_VIDEOBUFFER_H

#include "../../utils/BaseTypes.h"
#include <string>
#include <vector>

#define VIDEO_PIXEL_START                                                      \
  0x4000 // Start of the video videoBuffer in the memory map
#define VIDEO_ATTR_START 0x5800 // Start of the attribute data
#define VIDEO_BITMAP_DATA 6144  // Number of bytes of bitmap data
#define VIDEO_ATTR_DATA 768     // NUmber of bytes of colour data
#define VIDEO_WIDTH_CHARS 32    // Width of the attribute character map
#define VIDEO_HEIGHT_CHARS 24   // Height of the attribute character map

/**
 * The ZX Spectrum video layout
 * 1. 6144 bytes of data starting at 0x4000
 * 2. 768 bytes of colour attribute data starting at 0x5800
 *
 * Screen resolution = 256*192
 *
 * 192 lines of 32 bytes
 * Pixels are encoded as bits so 32*8=256 pixels
 *
 * https://www.overtakenbyevents.com/lets-talk-about-the-zx-specrum-screen-layout/#:~:text=The%20spectrum's%20screen%20memory%20starts,32%20bytes%20by%20192%20rows).
 */
#define BYTES_PER_ROW 32

class VideoBuffer {
private:
  emulator_types::byte *videoBuffer;
  emulator_types::byte *colourAttributes;
  emulator_types::byte borderColor = 7; // Default white border
  std::vector<emulator_types::byte> scanlineBorderColors;

  emulator_types::word encodeAddress(int x, int y) const;
  void printBits(std::string msg, size_t const size,
                 void const *const ptr) const;

public:
  explicit VideoBuffer(emulator_types::byte *memoryMap);

  emulator_types::byte getByte(int x, int y) const;
  void setByte(int x, int y, emulator_types::byte);
  emulator_types::byte getAttribute(int x, int y) const;

  void setBorderColor(emulator_types::byte color) {
    borderColor = color & 0x07;
    // Fallback: fill all?
    std::fill(scanlineBorderColors.begin(), scanlineBorderColors.end(),
              borderColor);
  }

  void setBorderColor(emulator_types::byte color, long tStates);
  void newFrame();

  emulator_types::byte getBorderColor() const { return borderColor; }
  emulator_types::byte getBorderColorAtLine(int line) const;

  // Override operators
  emulator_types::byte &operator[](int index);

  // Get a pointer to the start of the requested row
  emulator_types::byte *getBuffer() const { return videoBuffer; };
};

#endif // ZXEMULATOR_VIDEOBUFFER_H
