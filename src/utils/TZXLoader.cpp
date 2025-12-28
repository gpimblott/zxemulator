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
  if (this->size >= 10 && memcmp(this->data, expected, 7) == 0 &&
      this->data[7] == 0x1A) {
    return true;
  }

  // Try to validate as TAP
  // TAP format: [LEN_LO] [LEN_HI] [DATA...] repeated
  long offset = 0;
  bool validTap = false;
  while (offset < this->size) {
    if (offset + 2 > this->size)
      return false;
    int len = this->data[offset] | (this->data[offset + 1] << 8);
    offset += 2;
    if (offset + len > this->size)
      return false;
    offset += len;
    validTap = true;
  }
  return validTap;
}

void TZXLoader::parse() {
  if (!isValid()) {
    Logger::write("Invalid TZX file");
    return;
  }

  // Check format again to decide parsing strategy
  const char *expected = "ZXTape!";
  bool isTzx = (this->size >= 10 && memcmp(this->data, expected, 7) == 0);

  if (!isTzx) {
    // Parse as TAP
    Logger::write("Parsing as TAP format");
    long offset = 0;
    while (offset < this->size) {
      int len = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      TapeBlock block;
      block.id = 0x10;         // Standard Block
      block.pauseAfter = 1000; // Default pause for TAP blocks (1s)
      block.data.assign(this->data + offset, this->data + offset + len);
      blocks.push_back(block);

      offset += len;
    }
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
    } else if (blockId == 0x11) {
      // Turbo Speed Data Block (like 0x10 but with custom timing)
      // Header: 13 bytes timing + 2 bytes pause + 3 bytes length = 18 bytes
      if (offset + 18 > this->size)
        break;

      // Skip pilot pulse length (2), sync1(2), sync2(2), bit0(2), bit1(2)
      // pilot tone (2), last byte bits (1) = 13 bytes
      offset += 13;

      // Pause after block (2 bytes)
      int pause = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // Data length (3 bytes, but we'll use 2 for simplicity - files >64KB
      // rare)
      int length = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 3; // Skip all 3 bytes

      if (offset + length > this->size) {
        Logger::write("Block length exceeds file size");
        break;
      }

      TapeBlock block;
      block.id = 0x11;
      block.pauseAfter = pause;
      block.data.assign(this->data + offset, this->data + offset + length);
      blocks.push_back(block);

      char msg[100];
      snprintf(msg, sizeof(msg), "Block 0x11: Found %d bytes (turbo)", length);
      Logger::write(msg);

      offset += length;
    } else if (blockId == 0x12) {
      // Pure Tone - same pulse repeated N times
      // 0x00-0x01: Pulse length
      // 0x02-0x03: Number of pulses
      if (offset + 4 > this->size)
        break;

      offset += 4;
      Logger::write("Block 0x12: Pure Tone (skipped)");
    } else if (blockId == 0x13) {
      // Pulse Sequence - direct pulse lengths
      // 0x00: Number of pulses (N)
      // 0x01-...: N * 2 bytes of pulse lengths
      if (offset + 1 > this->size)
        break;

      byte numPulses = this->data[offset++];
      int pulseDataSize = numPulses * 2;

      if (offset + pulseDataSize > this->size)
        break;

      offset += pulseDataSize;
      Logger::write("Block 0x13: Pulse Sequence (skipped)");
    } else if (blockId == 0x20) {
      // Pause (Silence) or Stop Tape command
      // 0x00-0x01: Pause duration in ms (0 = stop tape)
      if (offset + 2 > this->size)
        break;

      word pauseDuration = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // We can ignore pause blocks for now as they're just timing hints
      // between blocks. A pause of 0 means "stop the tape" but for emulation
      // we can just continue to the next block.
      char msg[100];
      snprintf(msg, sizeof(msg), "Block 0x20: Pause %d ms", pauseDuration);
      Logger::write(msg);
    } else if (blockId == 0x21) {
      // Group Start - 1 byte length + N bytes name
      if (offset + 1 > this->size)
        break;

      byte nameLength = this->data[offset++];

      if (offset + nameLength > this->size)
        break;

      offset += nameLength;
      Logger::write("Block 0x21: Group Start (skipped)");
    } else if (blockId == 0x22) {
      // Group End - no data
      Logger::write("Block 0x22: Group End (skipped)");
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
    } else if (blockId == 0x32) {
      // Archive Info Block
      // 0x00-0x01: Block length (N)
      // 0x02: Number of text strings
      // Then N-1 bytes of text data
      if (offset + 2 > this->size)
        break;

      word blockLength = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      if (offset + blockLength > this->size)
        break;

      // Skip the archive info data - we don't need to parse it for emulation
      offset += blockLength;

      Logger::write("Block 0x32: Archive Info (skipped)");
    } else if (blockId == 0x24) {
      // Loop Start - 2 bytes: repeat count
      if (offset + 2 > this->size)
        break;
      offset += 2;
      Logger::write("Block 0x24: Loop Start (skipped)");
    } else if (blockId == 0x25) {
      // Loop End - no data
      Logger::write("Block 0x25: Loop End (skipped)");
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
