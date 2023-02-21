// File: (LogicFactory.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_LOGICFACTORY_H
#define ZXEMULATOR_LOGICFACTORY_H

#include "../../utils/BaseTypes.h"
#include "OpCode.h"
#include "OpCodeFactory.h"

namespace OpCodes {

    class LogicFactory : public OpCodeFactory {
    private:
        emulator_types::byte XOR_A = 0xaf;

        static int processXOR_A(ProcessorState &state);

    public:
        LogicFactory(ProcessorState &state);

        void addToCatalogue(OpCodeCatalogue &catalogue) override;
    };

} // OpCodes

#endif //ZXEMULATOR_LOGICFACTORY_H
