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

struct TapeBlock {
  int id;
  std::vector<emulator_types::byte> data;
  int pauseAfter; // block ID 0x10 usually has a pause
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
