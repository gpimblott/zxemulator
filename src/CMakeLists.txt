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
    spectrum/Memory.cpp spectrum/Memory.h
    exceptions/MemoryException.h
    spectrum/Processor.cpp
    spectrum/Processor_Index.cpp
    spectrum/Processor_Extended.cpp
    spectrum/Processor_Ops.cpp
    spectrum/video/Screen.cpp spectrum/video/Screen.h
    spectrum/video/windows/WindowsScreen.cpp spectrum/video/windows/WindowsScreen.h
    spectrum/video/VideoBuffer.cpp spectrum/video/VideoBuffer.h
    utils/PeriodTimer.cpp utils/PeriodTimer.h
    utils/debug.h
    spectrum/ProcessorMacros.h
    spectrum/ProcessorState.cpp spectrum/ProcessorState.h
    spectrum/Tape.cpp spectrum/Tape.h
    utils/TZXLoader.cpp utils/TZXLoader.h
    spectrum/Keyboard.cpp spectrum/Keyboard.h
    spectrum/Audio.cpp spectrum/Audio.h
    spectrum/SnapshotLoader.cpp spectrum/SnapshotLoader.h
    spectrum/TapeLoader.cpp spectrum/TapeLoader.h)

add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${ZX_SOURCES})

# Bundle Properties
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "ZXEmulator"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.pimblott.zxemulator"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.2.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.2"
    MACOSX_BUNDLE_ICON_FILE "icon.icns" # Optional: Add icon if available
)

# Detect and add SFML
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Network Audio)

if(SFML_FOUND)
    target_link_libraries(${CMAKE_PROJECT_NAME} SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio)
else()
    target_link_libraries(${CMAKE_PROJECT_NAME})
endif()

add_subdirectory(tests)

# Install Application Bundle
install(TARGETS ${EXECUTABLE_NAME}
    BUNDLE DESTINATION .
)

# Install Resources (ROMs) into the Bundle
install(DIRECTORY "${CMAKE_SOURCE_DIR}/../roms"
    DESTINATION "${EXECUTABLE_NAME}.app/Contents/Resources"
)

# CPack Configuration
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "ZXEmulator")
    set(CPACK_NSIS_PACKAGE_NAME "ZXEmulator")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "ZXEmulator")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/../resources/icon.ico") # Optional
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/../resources/icon.ico") # Optional
    set(CPACK_PACKAGE_FILE_NAME "ZXEmulator-0.2.0-win64")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "ZXEmulator Installer")
    set(CPACK_PACKAGE_FILE_NAME "ZXEmulator-0.2.0-macOS")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_PACKAGE_FILE_NAME "ZXEmulator-0.2.0-Linux")
    # Debian Specifics
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "gpimblott <gpimblott@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # Calculate dependencies automatically
endif()

include(CPack)



