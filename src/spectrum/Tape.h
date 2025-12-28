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

#ifndef ZXEMULATOR_TAPE_H
#define ZXEMULATOR_TAPE_H

#include "../utils/BaseTypes.h"
#include "../utils/TZXLoader.h"
#include <string>
#include <vector>

class Tape {
private:
  std::string filename;
  bool playing = false;
  std::vector<utils::TapeBlock> blocks;

  // Playback state
  enum TapeState { STOPPED, PILOT, SYNC1, SYNC2, DATA, PAUSE };
  TapeState currentState = STOPPED;
  size_t currentBlockIndex = 0;
  size_t currentByteIndex = 0;
  int currentBitIndex = 0;
  int pulseCount = 0;
  long tStateCounter = 0;
  long nextEdgeTState = 0;
  bool earBit = false;

public:
  Tape();

  void setFilename(const std::string &fn) { filename = fn; }
  void setBlocks(const std::vector<utils::TapeBlock> &blks) { blocks = blks; }
  void play();
  void stop();

  // Returns true if the EAR bit should be high (1) or low (0)
  // For now, this will be a stub to verify connection
  bool getEarBit();

  void update(int tStates);

  bool isPlaying() const { return playing; }

  // Fast Load Support
  // Returns true if block found and loaded, false otherwise
  bool fastLoadBlock(emulator_types::byte expectedFlag,
                     emulator_types::word length,
                     emulator_types::word startAddress, class Memory &memory);

  bool hasBlocks() const { return !blocks.empty(); }

  // Returns true if tape has reached the end (all blocks loaded)
  bool isFinished() const { return currentBlockIndex >= blocks.size(); }
};

#endif // ZXEMULATOR_TAPE_H
