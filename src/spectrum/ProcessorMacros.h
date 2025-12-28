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

#ifndef ZXEMULATOR_PROCESSORMACROS_H
#define ZXEMULATOR_PROCESSORMACROS_H

// Flag register macros

#define C_FLAG 0b00000001
#define N_FLAG 0b00000010
#define P_FLAG 0b00000100
#define X_FLAG 0b00001000
#define H_FLAG 0b00010000
#define Y_FLAG 0b00100000
#define Z_FLAG 0b01000000
#define S_FLAG 0b10000000

#define CLEAR_FLAG(flag, registers) (registers.F &= ~flag)
#define SET_FLAG(flag, registers) (registers.F |= flag)
#define GET_FLAG(flag, registers) (registers.F & flag)

#endif // ZXEMULATOR_PROCESSORMACROS_H
