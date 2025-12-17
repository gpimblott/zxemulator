cmake_minimum_required(VERSION 3.24)
project(ZXEmulator)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

# Need to ensure that the SFML libraries are included in the runtime path
# set(MY_DEPENDANCIES_PATHS /SFML/SFML-2.5.1/bin) # Removed legacy path
# set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake_modules" ${CMAKE_MODULE_PATH})

add_executable(${EXECUTABLE_NAME} main.cpp utils/Logger.cpp utils/Logger.h utils/BinaryFileLoader.cpp utils/BinaryFileLoader.h utils/BaseTypes.h utils/RegisterUtils.cpp utils/RegisterUtils.h spectrum/Rom.cpp spectrum/Rom.h spectrum/Memory.cpp spectrum/Memory.h spectrum/Opcodes/OpCode.cpp spectrum/Opcodes/OpCode.h exceptions/MemoryException.h spectrum/Processor.cpp spectrum/Processor.h spectrum/Opcodes/OpCodeCatalogue.cpp spectrum/Opcodes/OpCodeCatalogue.h spectrum/Opcodes/instructions/InterruptOpcodes.cpp spectrum/Opcodes/instructions/InterruptOpcodes.h spectrum/Opcodes/instructions/LoadOpcodes.cpp spectrum/Opcodes/instructions/LoadOpcodes.h spectrum/Opcodes/instructions/LogicOpcodes.cpp spectrum/Opcodes/instructions/LogicOpcodes.h spectrum/ProcessorTypes.h spectrum/Opcodes/OpCodeProvider.cpp spectrum/Opcodes/OpCodeProvider.h spectrum/video/Screen.cpp spectrum/video/Screen.h spectrum/video/windows/WindowsScreen.cpp spectrum/video/windows/WindowsScreen.h spectrum/video/VideoBuffer.cpp spectrum/video/VideoBuffer.h utils/PeriodTimer.cpp utils/PeriodTimer.h utils/debug.h    spectrum/Opcodes/instructions/JumpOpcodes.cpp spectrum/Opcodes/instructions/JumpOpcodes.h spectrum/ProcessorMacros.h spectrum/ProcessorState.cpp spectrum/ProcessorState.h spectrum/Opcodes/instructions/IOOpcodes.cpp spectrum/Opcodes/instructions/IOOpcodes.h    spectrum/Opcodes/ExtendedOpcodes.cpp
    spectrum/Opcodes/IndexOpcodes.cpp
    spectrum/Opcodes/instructions/StackOpcodes.cpp
    spectrum/Opcodes/instructions/BitOpcodes.cpp spectrum/Opcodes/ExtendedOpcodes.h spectrum/Opcodes/instructions/ArithmeticOpcodes.cpp spectrum/Opcodes/instructions/ArithmeticOpcodes.h spectrum/Opcodes/instructions/RotateOpcodes.cpp spectrum/Opcodes/instructions/RotateOpcodes.h)


# Detect and add SFML
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Network Audio)

if(SFML_FOUND)
    target_link_libraries(${CMAKE_PROJECT_NAME} SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio)
else()
    target_link_libraries(${CMAKE_PROJECT_NAME})
endif()

add_subdirectory(tests)



