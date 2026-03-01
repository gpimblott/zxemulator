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
      block.id = 0x10; // Standard Block
      block.type = STANDARD_SPEED_DATA;
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
      int pause = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;
      int length = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;
      if (offset + length > this->size)
        break;

      TapeBlock block;
      block.id = 0x10;
      block.type = STANDARD_SPEED_DATA;
      block.pauseAfter = pause;
      block.data.assign(this->data + offset, this->data + offset + length);
      blocks.push_back(block);
      offset += length;

    } else if (blockId == 0x11) { // Turbo Speed Data Block
      if (offset + 18 > this->size)
        break;

      TapeBlock block;
      block.id = 0x11;
      block.type = TURBO_SPEED_DATA;

      // 00-01: Pulse length of Pilot Pulse
      block.pilotPulseLen = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 02-03: Pulse length of Sync1
      block.sync1Len = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 04-05: Pulse length of Sync2
      block.sync2Len = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 06-07: Pulse length of Zero bit
      block.zeroLen = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 08-09: Pulse length of One bit
      block.oneLen = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 0A-0B: Pilot Tone Length (number of pulses)
      block.pilotToneLen = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      // 0C: Used bits in last byte
      block.lastByteUsedBits = this->data[offset];
      offset += 1;

      // 0D-0E: Pause after block
      int pause = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;
      block.pauseAfter = pause;

      // 0F-11: Data length (3 bytes)
      int length = this->data[offset] | (this->data[offset + 1] << 8) |
                   (this->data[offset + 2] << 16);
      offset += 3;

      if (offset + length > this->size)
        break;

      block.data.assign(this->data + offset, this->data + offset + length);
      blocks.push_back(block);
      offset += length;
      Logger::write("Block 0x11: Turbo Speed Data");

    } else if (blockId == 0x12) { // Pure Tone
      if (offset + 4 > this->size)
        break;
      int pulseLen = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;
      int pulseCnt = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      TapeBlock block;
      block.id = 0x12;
      block.type = PURE_TONE;
      block.pulseLength = pulseLen;
      block.pulseCount = pulseCnt;
      blocks.push_back(block);
      Logger::write("Block 0x12: Pure Tone");

    } else if (blockId == 0x13) { // Pulse Sequence
      if (offset + 1 > this->size)
        break;
      int seqCount = this->data[offset++];
      if (offset + seqCount * 2 > this->size)
        break;

      TapeBlock block;
      block.id = 0x13;
      block.type = PULSE_SEQUENCE;
      for (int i = 0; i < seqCount; i++) {
        int p = this->data[offset] | (this->data[offset + 1] << 8);
        offset += 2;
        block.pulseSequence.push_back(p);
      }
      blocks.push_back(block);
      Logger::write("Block 0x13: Pulse Sequence");

    } else if (blockId == 0x20) { // Pause / Stop
      if (offset + 2 > this->size)
        break;
      int pause = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;

      TapeBlock block;
      block.id = 0x20;
      block.type = PAUSE;
      block.pauseAfter = pause;
      blocks.push_back(block);
      // Removed log

    } else if (blockId == 0x21) {
      // Group Start
      byte len = this->data[offset++];
      offset += len;
      TapeBlock block;
      block.type = GROUP_START;
      blocks.push_back(block);
    } else if (blockId == 0x22) {
      // Group End
      TapeBlock block;
      block.type = GROUP_END;
      blocks.push_back(block);
    } else if (blockId == 0x24) {
      // Loop Start
      int loopCount = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;
      TapeBlock block;
      block.type = LOOP_START;
      block.pulseCount = loopCount; // Use pulseCount field for loop count
      blocks.push_back(block);
    } else if (blockId == 0x25) {
      // Loop End
      TapeBlock block;
      block.type = LOOP_END;
      blocks.push_back(block);
    } else if (blockId == 0x14) { // Pure Data Block
      // 00-01: Zero bit pulse len
      // 02-03: One bit pulse len
      // 04:    Last byte used bits
      // 05-06: Pause after
      // 07-09: Length (3 bytes)
      if (offset + 10 > this->size)
        break;

      TapeBlock block;
      block.id = 0x14;
      block.type = TURBO_SPEED_DATA;
      block.zeroLen = this->data[offset] | (this->data[offset + 1] << 8);
      block.oneLen = this->data[offset + 2] | (this->data[offset + 3] << 8);

      // Sanitize: If lengths are 0, use Standard defaults
      if (block.zeroLen == 0)
        block.zeroLen = 855;
      if (block.oneLen == 0)
        block.oneLen = 1710;

      block.lastByteUsedBits = this->data[offset + 4];
      block.pauseAfter = this->data[offset + 5] | (this->data[offset + 6] << 8);

      uint32_t len = this->data[offset + 7] | (this->data[offset + 8] << 8) |
                     (this->data[offset + 9] << 16);
      offset += 10;

      if (offset + len > this->size)
        len = this->size - offset;

      // Pure Data has NO Pilot and NO Sync
      block.pilotToneLen = 0;
      block.pilotPulseLen = 0;
      block.sync1Len = 0;
      block.sync2Len = 0;

      if (len > 0) {
        block.data.insert(block.data.end(), this->data + offset,
                          this->data + offset + len);
        offset += len;
      }
      blocks.push_back(block);
      // Removed log

    } else if (blockId == 0x30) {
      byte len = this->data[offset++];
      offset += len;
    } else if (blockId == 0x32 || blockId == 0x33) { // Archive Info
      int len = this->data[offset] | (this->data[offset + 1] << 8);
      offset += 2;
      offset += len;
    } else {
      // Unknown - abort?
      char msg[64];
      snprintf(msg, sizeof(msg),
               "Unknown Block %02X at offset %zu - Parsing stopped", blockId,
               (size_t)offset);
      Logger::write(msg);
      break;
    }
  }
  char summary[64];
  snprintf(summary, sizeof(summary), "TZX Parse Completed. Found %zu blocks.",
           blocks.size());
  Logger::write(summary);
}
