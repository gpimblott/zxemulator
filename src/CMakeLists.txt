cmake_minimum_required(VERSION 3.20)
project(ZXEmulator)

set(EXECUTABLE_NAME ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED ZX_VERSION)
    set(ZX_VERSION "0.0.1")
endif()

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

if(APPLE)
    list(APPEND ZX_SOURCES platform/mac/MacFileOpenHandler.mm)
endif()

# Collect Resource Files
file(GLOB ROM_FILES "${CMAKE_SOURCE_DIR}/../roms/*")
set(ICON_FILE "${CMAKE_SOURCE_DIR}/../resources/icon.icns")

# Set Bundle Location Properties
set_source_files_properties(${ROM_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/roms)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${ZX_SOURCES} ${ROM_FILES} ${ICON_FILE})

# Bundle Properties
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "ZXEmulator"
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2025 G.Pimblott"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.pimblott.zxemulator"
    MACOSX_BUNDLE_BUNDLE_VERSION "${ZX_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${ZX_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_ICON_FILE "icon.icns"
)

# Post-build: Copy executable to build root for convenience
add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${EXECUTABLE_NAME}>
    ${CMAKE_BINARY_DIR}/${EXECUTABLE_NAME}
    COMMENT "Copying executable to build directory..."
)

# Detect dependencies
if(UNIX AND NOT APPLE)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Network Audio)

if(SFML_FOUND)
    if(APPLE)
        target_link_libraries(${CMAKE_PROJECT_NAME} SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio "-framework Cocoa")
    else()
        target_link_libraries(${CMAKE_PROJECT_NAME} SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio)
    endif()
else()
    target_link_libraries(${CMAKE_PROJECT_NAME})
endif()

add_subdirectory(tests)

# Install Application Bundle
# Install Application Bundle
if(APPLE)
    install(TARGETS ${EXECUTABLE_NAME} BUNDLE DESTINATION .)
    # Install Resources (ROMs) into the Bundle
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/../roms" DESTINATION "${EXECUTABLE_NAME}.app/Contents/Resources")
    # Install Icon into Bundle
    install(FILES "${CMAKE_SOURCE_DIR}/../resources/icon.icns" DESTINATION "${EXECUTABLE_NAME}.app/Contents/Resources")

    # Fixup Bundle (Copy Dependencies) and Ad-Hoc Sign
    install(CODE "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${EXECUTABLE_NAME}.app\" \"\" \"\")
        execute_process(COMMAND codesign --force --deep --sign - \"\${CMAKE_INSTALL_PREFIX}/${EXECUTABLE_NAME}.app\")
    ")
else()
    install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/../roms" DESTINATION share/zxemulator/resources)
    # Install Runtime Icon
    install(FILES "${CMAKE_SOURCE_DIR}/../resources/icon.png" DESTINATION share/zxemulator/resources) 
    if(UNIX)
        install(FILES "${CMAKE_SOURCE_DIR}/../resources/icon.png" DESTINATION share/pixmaps)
    endif()
endif()

# CPack Configuration
set(CPACK_PACKAGE_VENDOR "G.Pimblott")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ZX Spectrum Emulator")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/../LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "ZXEmulator")
    set(CPACK_NSIS_PACKAGE_NAME "ZXEmulator")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "ZXEmulator")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/../resources/icon.ico") # Optional
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/../resources/icon.ico") # Optional
    set(CPACK_PACKAGE_FILE_NAME "ZXEmulator-${ZX_VERSION}-win64")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "ZXEmulator Installer")
    set(CPACK_MACOSX_BUNDLE_ICON_FILE "icon.icns")
    set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/../resources/icon.icns")
    set(CPACK_PACKAGE_FILE_NAME "ZXEmulator-${ZX_VERSION}-macOS")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_PACKAGE_FILE_NAME "ZXEmulator-${ZX_VERSION}-Linux")
    # Debian Specifics
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "gpimblott <gordon@pimblott.com>")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # Calculate dependencies automatically
endif()

include(CPack)



