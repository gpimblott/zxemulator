/*
 * MIT License
 *
 * Copyright (c) 2026 G.Pimblott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "OpCode.h"

/**
 * Constructor to create a specific opcode
 * @param opcode instruction code
 * @param name text name of the instruction
 * @param func pointer to a function to execute to handle this opcode
 */
OpCode::OpCode(emulator_types::byte opcode, std::string name,
               executeFunc_t func)
    : code(opcode), name(name), executeFunc(func) {}

/**
 * Retrieve the name of he instruction
 * @return The string name of the instruction
 */
std::string OpCode::getName() { return this->name; }

/**
 * Get the byte opcode for the instruction
 * @return The byte value for the instruction
 */
emulator_types::byte OpCode::getOpCode() { return this->code; }

/**
 * Execute the code associated with this opcode
 * @return status code
 */
int OpCode::execute(ProcessorState &state) { return this->executeFunc(state); }
