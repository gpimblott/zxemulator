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

#ifndef ZXEMULATOR_BINARYFILELOADER_H
#define ZXEMULATOR_BINARYFILELOADER_H

#include "BaseTypes.h"
#include <string>

/**
 *
 */
class BinaryFileLoader {
protected:
  const char *filename = 0;
  long size = 0;
  emulator_types::byte *data = 0;

public:
  BinaryFileLoader(const char *filename);
  BinaryFileLoader(const char *filename, emulator_types::byte *buffer);
  emulator_types::byte operator[](long i);

  // Utility routines to load a file
  static long getFileSize(const char *filename);
  static void readFileToBuffer(const char *filename,
                               emulator_types::byte *buffer, int size);
};

#endif // ZXEMULATOR_BINARYFILELOADER_H
