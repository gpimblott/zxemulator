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

#ifndef ZXEMULATOR_SNAPSHOTLOADER_H
#define ZXEMULATOR_SNAPSHOTLOADER_H

#include "ProcessorState.h"

class SnapshotLoader {
public:
  static void load(const char *filename, ProcessorState &state);
  static void exportSNA(const char *filename, ProcessorState &state);

private:
  static void loadSNA(const char *filename, ProcessorState &state);
  static void loadZ80(const char *filename, ProcessorState &state);
};

#endif // ZXEMULATOR_SNAPSHOTLOADER_H
