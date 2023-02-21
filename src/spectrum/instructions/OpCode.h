//
// Created by gordo on 17/01/2023.
//

#ifndef ZXEMULATOR_OPCODE_H
#define ZXEMULATOR_OPCODE_H

#include <string>
#include "../../utils/BaseTypes.h"
#include "../ProcessorTypes.h"

// Define the type of the execution function
typedef int (*executeFunc_t)(ProcessorState&);

/**
 * Representation of a single instruction opcode recognised by the processor
 * This base class is extended by each specific instruction
 */
class OpCode {
private:
    emulator_types::byte code;
    std::string name;
    ProcessorState &state;
    executeFunc_t executeFunc;

public:
    // Constructor
    OpCode(ProcessorState &state, emulator_types::byte opcode, std::string name, executeFunc_t func);

    // Method that implements the specific code for an instruction
    int execute();

    // Methods
    emulator_types::byte getOpCode();
    std::string getName();
};


#endif //ZXEMULATOR_OPCODE_H
