/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TZXLoader.h"
#include "Logger.h"
#include <cstdio>
#include <cstring>

using namespace utils;
using namespace emulator_types;

TZXLoader::TZXLoader(const char *filename) : BinaryFileLoader(filename) {}

bool TZXLoader::isValid() {
  if (this->size < 10)
    return false;

  // TZX Header: "ZXTape!" + 0x1A
  const char *expected = "ZXTape!";
  if (memcmp(this->data, expected, 7) != 0)
    return false;
  if (this->data[7] != 0x1A)
    return false;

  return true;
}

void TZXLoader::parse() {
  if (!isValid()) {
    Logger::write("Invalid TZX file");
    return;
  }

  int major = this->data[8];
  int minor = this->data[9];
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "TZX Version: %d.%d", major, minor);
  Logger::write(buffer);

  // Block parsing would start at offset 10
  long offset = 10;
  while (offset < this->size) {
    byte blockId = this->data[offset++];

    if (blockId == 0x10) { // Standard Speed Data Block
      if (offset + 4 > this->size)
        break;

      // 0x00-0x01: Pause after this block (ms)
      int pause = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 0x02-0x03: Length of data block
      int length = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      if (offset + length > this->size) {
        Logger::write("Block length exceeds file size");
        break;
      }

      TapeBlock block;
      block.id = 0x10;
      block.pauseAfter = pause;
      block.data.assign(this->data + offset, this->data + offset + length);

      blocks.push_back(block);

      char msg[100];
      snprintf(msg, sizeof(msg), "Block 0x10: Found %d bytes", length);
      Logger::write(msg);

      offset += length;
    } else if (blockId == 0x30) {
      // Text Description Block
      // 0x00: Length (N)
      // 0x01..N: Text
      if (offset + 1 > this->size)
        break;
      byte length = this->data[offset++];

      if (offset + length > this->size)
        break;

      std::string text((char *)(this->data + offset), length);
      Logger::write(("TZX Info: " + text).c_str());

      offset += length;
    } else {
      // Unknown block, abort for safety or skip if length known?
      // TZX structure is complex, for MVP we abort on unknown to verify 0x10
      // support first
      char msg[100];
      snprintf(msg, sizeof(msg), "Unknown Block ID: %02X at offset %ld",
               blockId, offset - 1);
      Logger::write(msg);
      break;
    }
  }
}
