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

#include "Rom.h"

/**
 * Constructor that loads the specified filename
 * @param filename
 */
Rom::Rom(const char *filename) : BinaryFileLoader(filename) {}

Rom::Rom(const char *filename, emulator_types::byte *buffer)
    : BinaryFileLoader(filename, buffer) {}

long Rom::getSize() { return this->size; }

emulator_types::byte *Rom::getData() { return this->data; }
