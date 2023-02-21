cmake_minimum_required(VERSION 3.24)
project(ZXEmulator)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

# Need to ensure that the SFML libraries are included in the runtime path
set(MY_DEPENDANCIES_PATHS /SFML/SFML-2.5.1/bin)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake_modules" ${CMAKE_MODULE_PATH})

add_executable(${EXECUTABLE_NAME} main.cpp utils/Logger.cpp utils/Logger.h utils/BinaryFileLoader.cpp utils/BinaryFileLoader.h utils/BaseTypes.h utils/RegisterUtils.cpp utils/RegisterUtils.h spectrum/Rom.cpp spectrum/Rom.h spectrum/Memory.cpp spectrum/Memory.h spectrum/instructions/OpCode.cpp spectrum/instructions/OpCode.h exceptions/MemoryException.h spectrum/Processor.cpp spectrum/Processor.h spectrum/instructions/OpCodeCatalogue.cpp spectrum/instructions/OpCodeCatalogue.h spectrum/instructions/InterruptsFactory.cpp spectrum/instructions/InterruptsFactory.h spectrum/instructions/LoadFactory.cpp spectrum/instructions/LoadFactory.h spectrum/instructions/LogicFactory.cpp spectrum/instructions/LogicFactory.h spectrum/ProcessorTypes.h spectrum/instructions/OpCodeFactory.cpp spectrum/instructions/OpCodeFactory.h spectrum/video/Screen.cpp spectrum/video/Screen.h spectrum/video/windows/WindowsScreen.cpp spectrum/video/windows/WindowsScreen.h spectrum/video/VideoBuffer.cpp spectrum/video/VideoBuffer.h utils/PeriodTimer.cpp utils/PeriodTimer.h)

# Detect and add SFML
find_package(SFML 2 REQUIRED system window graphics network audio)
if(SFML_FOUND)
    include_directories(${SFML_INCLUDE_DIR})
    message(STATUS ${SFML_LIBRARIES})
    target_link_libraries(${CMAKE_PROJECT_NAME} ${SFML_LIBRARIES})
else()
    target_link_libraries(${CMAKE_PROJECT_NAME})
endif()
