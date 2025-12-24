cmake_minimum_required(VERSION 4.0)
project(ZXEmulator)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Need to ensure that the SFML libraries are included in the runtime path
# set(MY_DEPENDANCIES_PATHS /SFML/SFML-2.5.1/bin) # Removed legacy path
# set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake_modules" ${CMAKE_MODULE_PATH})

set(ZX_SOURCES
    main.cpp
    utils/Logger.cpp utils/Logger.h
    utils/BinaryFileLoader.cpp utils/BinaryFileLoader.h
    utils/BaseTypes.h
    utils/RegisterUtils.cpp utils/RegisterUtils.h
    spectrum/Rom.cpp spectrum/Rom.h
    spectrum/Memory.cpp spectrum/Memory.h
    spectrum/Opcodes/OpCode.cpp spectrum/Opcodes/OpCode.h
    exceptions/MemoryException.h
    spectrum/Processor.cpp    spectrum/Opcodes/OpCode.h
    spectrum/Opcodes/OpCodeCatalogue.cpp
    spectrum/Opcodes/OpCodeCatalogue.h
    spectrum/Opcodes/OpCodeProvider.cpp spectrum/Opcodes/OpCodeProvider.h
    spectrum/video/Screen.cpp spectrum/video/Screen.h
    spectrum/video/windows/WindowsScreen.cpp spectrum/video/windows/WindowsScreen.h
    spectrum/video/VideoBuffer.cpp spectrum/video/VideoBuffer.h
    utils/PeriodTimer.cpp utils/PeriodTimer.h
    utils/debug.h
    spectrum/ProcessorMacros.h
    spectrum/ProcessorState.cpp spectrum/ProcessorState.h
    spectrum/Opcodes/IndexOpcodes.cpp
    spectrum/ALUHelpers.cpp
    spectrum/Tape.cpp spectrum/Tape.h
    utils/TZXLoader.cpp utils/TZXLoader.h
    spectrum/Keyboard.cpp spectrum/Keyboard.h
    spectrum/Audio.cpp spectrum/Audio.h
    spectrum/SnapshotLoader.cpp spectrum/SnapshotLoader.h
    spectrum/TapeLoader.cpp spectrum/TapeLoader.h)

add_executable(${EXECUTABLE_NAME} ${ZX_SOURCES})


# Detect and add SFML
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Network Audio)

if(SFML_FOUND)
    target_link_libraries(${CMAKE_PROJECT_NAME} SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio)
else()
    target_link_libraries(${CMAKE_PROJECT_NAME})
endif()

add_subdirectory(tests)



