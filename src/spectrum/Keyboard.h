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

#ifndef ZXEMULATOR_KEYBOARD_H
#define ZXEMULATOR_KEYBOARD_H

#include "../utils/BaseTypes.h"
#include <vector>

class Keyboard {
private:
  // 8 rows (lines), 5 bits each.
  // Index 0 = 0xFE (SHIFT ... V)
  // Index 1 = 0xFD (A ... G)
  // ...
  // Store as bytes where 0 = pressed, 1 = released?
  // Usually it's easier to store 1=pressed internally and invert on read.
  // Let's store 1 = pressed for simplicity, return ~(pressed) masked.
  emulator_types::byte keyLines[8];

public:
  Keyboard();

  /**
   * Set the state of a specific key
   * @param line 0-7 (corresponding to high byte bit 0-7 being 0)
   * @param bit 0-4 (D0-D4)
   * @param pressed true if pressed
   */
  void setKey(int line, int bit, bool pressed);

  /**
   * Read the input port
   * @param highByte The high byte of the port address (e.g. 0xFE, 0xFD...)
   * @return The data byte (D0-D4 contain key status). Bits set to 1 if NOT
   * pressed.
   */
  emulator_types::byte readPort(emulator_types::byte highByte);

  // Reset all keys
  void reset();

  void setKempstonKey(int bit, bool pressed);
  emulator_types::byte readKempstonPort();

private:
  emulator_types::byte kempstonState;
};

#endif // ZXEMULATOR_KEYBOARD_H
