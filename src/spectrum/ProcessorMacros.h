// File: (ProcessorMacros.h)
// Created by G.Pimblott on 26/02/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

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

#define CLEAR_FLAG(flag, registers) (registers.flags &= ~flag)
#define SET_FLAG(flag, registers) (registers.flags |= flag)
#define GET_FLAG(flag, registers) (registers.flags & flag)


#endif //ZXEMULATOR_PROCESSORMACROS_H
