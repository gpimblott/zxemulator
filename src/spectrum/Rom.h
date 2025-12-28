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

#ifndef ZXEMULATOR_ROM_H
#define ZXEMULATOR_ROM_H

#include "../utils/BaseTypes.h"
#include "../utils/BinaryFileLoader.h"

class Rom : public BinaryFileLoader {

public:
  Rom(const char *filename);
  Rom(const char *filename, emulator_types::byte *buffer);

  long getSize();
  emulator_types::byte *getData();
};

#endif // ZXEMULATOR_ROM_H
