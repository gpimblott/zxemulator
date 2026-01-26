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

#ifndef ZXEMULATOR_TZXLOADER_H
#define ZXEMULATOR_TZXLOADER_H

#include "BinaryFileLoader.h"
#include <string>
#include <vector>

namespace utils {

enum BlockType {
  STANDARD_SPEED_DATA, // 0x10
  TURBO_SPEED_DATA,    // 0x11
  PURE_TONE,           // 0x12
  PULSE_SEQUENCE,      // 0x13
  PURE_DATA,           // 0x14
  DIRECT_RECORDING,    // 0x15
  PAUSE,               // 0x20
  GROUP_START,         // 0x21
  GROUP_END,           // 0x22
  LOOP_START,          // 0x24
  LOOP_END,            // 0x25
  UNKNOWN
};

struct TapeBlock {
  BlockType type = UNKNOWN;
  int id = 0;
  std::vector<emulator_types::byte> data; // For Standard/Turbo/PureData

  // Timing / Pulse info
  int pauseAfter = 0;                              // ms
  int pulseLength = 0;                             // T-states
  int pulseCount;                                  // For Pure Tone / Loop Count
  std::vector<emulator_types::word> pulseSequence; // For Block 0x13

  // Turbo Block Timings (0x11)
  int pilotPulseLen = 2168;
  int sync1Len = 667;
  int sync2Len = 735;
  int zeroLen = 855;
  int oneLen = 1710;
  int pilotToneLen = 3220; // Number of pulses
  int lastByteUsedBits = 8;
};

class TZXLoader : public BinaryFileLoader {
private:
  std::vector<TapeBlock> blocks;

public:
  TZXLoader(const char *filename);

  bool isValid();
  void parse();

  const std::vector<TapeBlock> &getBlocks() const { return blocks; }
};

} // namespace utils

#endif // ZXEMULATOR_TZXLOADER_H
