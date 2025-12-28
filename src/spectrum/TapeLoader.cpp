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

#include "TapeLoader.h"
#include "../utils/Logger.h"
#include "../utils/TZXLoader.h"

using namespace utils;

Tape TapeLoader::load(const char *filename) {
  std::string msg = "Loading tape: " + std::string(filename);
  Logger::write(msg.c_str());

  Tape tape;
  TZXLoader loader(filename);
  if (loader.isValid()) {
    loader.parse();
    tape.setBlocks(loader.getBlocks());
    tape.setFilename(filename);
  } else {
    Logger::write("Failed to load or invalid TZX file");
  }
  return tape;
}
