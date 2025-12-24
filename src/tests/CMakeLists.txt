# 'Google_test' is the subproject name
project(Google_tests)

cmake_minimum_required(VERSION 3.24)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 'lib' is the folder with Google Test sources
#add_subdirectory(lib)
# include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests

# Filter out main.cpp from ZX_SOURCES to avoid multiple main() definitions
set(ZX_TEST_SOURCES "")
foreach(SOURCE_FILE ${ZX_SOURCES})
    if(NOT SOURCE_FILE MATCHES "main.cpp")
        list(APPEND ZX_TEST_SOURCES "../${SOURCE_FILE}")
    endif()
endforeach()

add_executable(Google_Tests_run ProcessorTest.cpp)
target_link_libraries(Google_Tests_run gtest gtest_main)

add_executable(Instruction_Tests_run InstructionTest.cpp BenchmarkTest.cpp ${ZX_TEST_SOURCES})
target_link_libraries(Instruction_Tests_run gtest gtest_main SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio)