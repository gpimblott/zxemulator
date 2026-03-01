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

#ifndef ZXEMULATOR_AUDIODEVICE_H
#define ZXEMULATOR_AUDIODEVICE_H

#include <cstddef>

class AudioDevice {
public:
  virtual ~AudioDevice() = default;

  // Called by the Processor during instruction execution
  virtual void update(int tStates, bool speakerBit, bool earBit) = 0;

  // Core control functions
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void reset() = 0;
  virtual void flush() = 0;
  virtual size_t getBufferSize() = 0;
};

#endif // ZXEMULATOR_AUDIODEVICE_H
