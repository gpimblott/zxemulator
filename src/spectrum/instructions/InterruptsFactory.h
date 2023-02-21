// File: (InterruptsFactory.h)
// Created by G.Pimblott on 18/01/2023.
// Copyright (c) 2023 G.Pimblott All rights reserved.
//

#ifndef ZXEMULATOR_INTERRUPTSFACTORY_H
#define ZXEMULATOR_INTERRUPTSFACTORY_H

#include "../../utils/BaseTypes.h"
#include "OpCode.h"
#include "OpCodeFactory.h"

namespace OpCodes {

    class InterruptsFactory : public OpCodeFactory {
    private:
        static constexpr emulator_types::byte DI = 0xf3;
        static int processDI(ProcessorState &state);

    public:
        InterruptsFactory(ProcessorState &state);
        void addToCatalogue(OpCodeCatalogue &catalogue) override;
    };
}


#endif //ZXEMULATOR_INTERRUPTSFACTORY_H
