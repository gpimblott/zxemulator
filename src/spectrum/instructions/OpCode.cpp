//
// Created by G.Pimblott on 17/01/2023.
//
#include "OpCode.h"

/**
 * Constructor to create a specific opcode
 * @param state Reference to the processor state variables
 * @param opcode instruction code
 * @param name text name of the instruction
 * @param func pointer to a function to execute to handle this opcode
 */
OpCode::OpCode(ProcessorState &state,emulator_types::byte opcode, std::string name, executeFunc_t func)
        : state(state),code(opcode), name(name), executeFunc(func) {
}

/**
 * Retrieve the name of he instruction
 * @return The string name of the instruction
 */
std::string OpCode::getName() {
    return this->name;
}

/**
 * Get the byte opcode for the instruction
 * @return The byte value for the instruction
 */
emulator_types::byte OpCode::getOpCode() {
    return this->code;
}

/**
 * Execute the code associated with this opcode
 * @return status code
 */
int OpCode::execute() {
    return this->executeFunc( this->state );
}


