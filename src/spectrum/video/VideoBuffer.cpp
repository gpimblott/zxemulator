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

#include "VideoBuffer.h"
#include <cstring>

/**
 * Overlay the Video videoBuffer onto the main memory map
 * @param memoryMap A pointer to the memory map being used
 */
VideoBuffer::VideoBuffer(emulator_types::byte *memoryMap) {
  this->videoBuffer = memoryMap + VIDEO_PIXEL_START;
  this->colourAttributes = memoryMap + VIDEO_ATTR_START;

  // Blank memory out
  memset(videoBuffer, 0, VIDEO_BITMAP_DATA);
  // Set attributes to White Paper (7), Black Ink (0), Bright 0, Flash 0 -> 00
  // 111 000 -> 0x38
  memset(colourAttributes, 0x38, VIDEO_ATTR_DATA);

  scanlineBorderColors.resize(312, 7); // 312 lines (PAL), default white
}

void VideoBuffer::newFrame() {
  // Fill with current border color (carry over from previous frame)
  std::fill(scanlineBorderColors.begin(), scanlineBorderColors.end(),
            borderColor);
}

void VideoBuffer::setBorderColor(emulator_types::byte color, long tStates) {
  borderColor = color & 0x07;
  // 224 T-states per line
  int line = tStates / 224;
  if (line < 0)
    line = 0;
  if (line >= scanlineBorderColors.size())
    return;

  // Set from current line to end
  std::fill(scanlineBorderColors.begin() + line, scanlineBorderColors.end(),
            borderColor);
}

emulator_types::byte VideoBuffer::getBorderColorAtLine(int line) const {
  if (line < 0)
    return scanlineBorderColors[0];
  if (line >= scanlineBorderColors.size())
    return scanlineBorderColors.back();
  return scanlineBorderColors[line];
}

/**
 * The screen address is not linear and is encoded as follows
 *
 *            H            |           L
 * 15 14 13 12 11 10  9  8 |  7  6  5  4  3  2  1  0
 *  0  1  0 Y7 Y6 Y2 Y1 Y0 | Y5 Y4 Y3 X4 X3 X2 X1 X0
 *
 * Bits 13,14 & 15 encode the 0x4000 start address
 *
 * @param x X screen position
 * @param y Y screen position
 * @return The byte for the supplied position
 */
emulator_types::byte VideoBuffer::getByte(int x, int y) const {

  emulator_types::word address = encodeAddress(x, y);
  emulator_types::byte result = *(videoBuffer + address);

  return result;
}

/**
 * Set the byte at the current x,y position in the video videoBuffer
 * @param x X Position
 * @param y Y Position
 * @param data The byte to set the target to
 */
void VideoBuffer::setByte(int x, int y, emulator_types::byte data) {

  emulator_types::word address = encodeAddress(x, y);
  *(videoBuffer + address) = data;
}

/**
 * Get the colour attributes for the character at the given x,y position
 * @param x X Position
 * @param y Y Position
 * @return The colour attribute byte for the specified position
 */
emulator_types::byte VideoBuffer::getAttribute(int x, int y) const {
  // x is in character columns (0-31)
  // y is in pixel rows (0-191)

  // Address logic:
  // Attribute area starts at 0x1800 (offset from video start)
  // It is a linear buffer of 32x24 bytes.
  // Each byte controls an 8x8 pixel block.

  // Convert pixel y to character row
  int charRow = y >> 3; // y / 8

  // Offset = Row * 32 + Column
  int offset = (charRow << 5) + x; // charRow * 32 + x

  return colourAttributes[offset];
}

/**
 * Override the [] operator to allow direct access the the video videoBuffer
 * @param index
 * @return
 */
emulator_types::byte &VideoBuffer::operator[](int index) {
  if (index > VIDEO_BITMAP_DATA) {
    printf("Video videoBuffer index out of range\n");
    exit(0);
  }
  return videoBuffer[index];
}

/**
 * The memory layout is not linear - calculate the position in memory
 * given the x,y coordinates
 * @param x X Position
 * @param y Y Position
 * @return The address offset of the given x,y coordinates
 */
emulator_types::word VideoBuffer::encodeAddress(int x, int y) const {
  emulator_types::word address = 0x00;
  // copy X bits X0-X4 to 0-4 in the address
  address |= (x & 0b11111);

  // Copy Y bits Y3-Y5 to 5-7 in the address
  address |= ((y & 0b00111000) << 2);

  // Copy Y bits Y0-Y2 to 8-10 in the address
  address |= ((y & 0b00000111) << 8);

  // Copy Y bits Y6-Y7 to 11-12 in the address
  address |= ((y & 0b11000000) << 5);

  return address;
}

/**
 * Debug routine to output memory as a but pattern
 * @param msg The message to display before the pattern
 * @param size The number of bytes to display
 * @param ptr Pointer to the data to output
 */
void VideoBuffer::printBits(std::string msg, size_t const size,
                            void const *const ptr) const {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  printf("%s : ", msg.c_str());
  for (i = size - 1; i >= 0; i--) {
    printf(" ");
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  printf("\n");
}
