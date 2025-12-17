# define the SFML_STATIC macro if static build was chosen
if(SFML_STATIC_LIBRARIES)
    add_definitions(-DSFML_STATIC)
endif()

# define the list of search paths for headers and libraries
set(FIND_SFML_PATHS ${PROJECT_SOURCE_DIR}/../SFML/SFML-2.5.1)

# find the SFML include directory
find_path(SFML_INCLUDE_DIR SFML/Config.hpp
        PATH_SUFFIXES include
        PATHS ${FIND_SFML_PATHS})

# check the version number
set(SFML_VERSION_OK TRUE)
if(SFML_FIND_VERSION AND SFML_INCLUDE_DIR)
    # extract the major and minor version numbers from SFML/Config.hpp
    # we have to handle framework a little bit differently:
    if("${SFML_INCLUDE_DIR}" MATCHES "SFML.framework")
        set(SFML_CONFIG_HPP_INPUT "${SFML_INCLUDE_DIR}/Headers/Config.hpp")
    else()
        set(SFML_CONFIG_HPP_INPUT "${SFML_INCLUDE_DIR}/SFML/Config.hpp")
    endif()
    FILE(READ "${SFML_CONFIG_HPP_INPUT}" SFML_CONFIG_HPP_CONTENTS)
    STRING(REGEX REPLACE ".*#define SFML_VERSION_MAJOR ([0-9]+).*" "\\1" SFML_VERSION_MAJOR "${SFML_CONFIG_HPP_CONTENTS}")
    STRING(REGEX REPLACE ".*#define SFML_VERSION_MINOR ([0-9]+).*" "\\1" SFML_VERSION_MINOR "${SFML_CONFIG_HPP_CONTENTS}")
    STRING(REGEX REPLACE ".*#define SFML_VERSION_PATCH ([0-9]+).*" "\\1" SFML_VERSION_PATCH "${SFML_CONFIG_HPP_CONTENTS}")
    if (NOT "${SFML_VERSION_PATCH}" MATCHES "^[0-9]+$")
        set(SFML_VERSION_PATCH 0)
    endif()
    math(EXPR SFML_REQUESTED_VERSION "${SFML_FIND_VERSION_MAJOR} * 10000 + ${SFML_FIND_VERSION_MINOR} * 100 + ${SFML_FIND_VERSION_PATCH}")

    # if we could extract them, compare with the requested version number
    if (SFML_VERSION_MAJOR)
        # transform version numbers to an integer
        math(EXPR SFML_VERSION "${SFML_VERSION_MAJOR} * 10000 + ${SFML_VERSION_MINOR} * 100 + ${SFML_VERSION_PATCH}")

        # compare them
        if(SFML_VERSION LESS SFML_REQUESTED_VERSION)
            set(SFML_VERSION_OK FALSE)
        endif()
    else()
        # SFML version is < 2.0
        if (SFML_REQUESTED_VERSION GREATER 10900)
            set(SFML_VERSION_OK FALSE)
            set(SFML_VERSION_MAJOR 1)
            set(SFML_VERSION_MINOR x)
            set(SFML_VERSION_PATCH x)
        endif()
    endif()
endif()

set(SFML_FOUND TRUE)

foreach(FIND_SFML_COMPONENT ${SFML_FIND_COMPONENTS})
    string(TOLOWER ${FIND_SFML_COMPONENT} FIND_SFML_COMPONENT_LOWER)
    string(TOUPPER ${FIND_SFML_COMPONENT} FIND_SFML_COMPONENT_UPPER)
    set(FIND_SFML_COMPONENT_NAME sfml-${FIND_SFML_COMPONENT_LOWER})

    message(STATUS "Finding ${FIND_SFML_COMPONENT_NAME}")

    # no suffix for sfml-main, it is always a static library
    if(FIND_SFML_COMPONENT_LOWER STREQUAL "main")
        # release library
        find_library(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE
                NAMES ${FIND_SFML_COMPONENT_NAME}
                PATH_SUFFIXES lib64 lib
                PATHS ${FIND_SFML_PATHS})

        # debug library
        find_library(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG
                NAMES ${FIND_SFML_COMPONENT_NAME}-d
                PATH_SUFFIXES lib64 lib
                PATHS ${FIND_SFML_PATHS})
    else()
        # static release library
        find_library(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_RELEASE
                NAMES ${FIND_SFML_COMPONENT_NAME}-s
                PATH_SUFFIXES lib64 lib
                PATHS ${FIND_SFML_PATHS})

        # static debug library
        find_library(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_DEBUG
                NAMES ${FIND_SFML_COMPONENT_NAME}-s-d
                PATH_SUFFIXES lib64 lib
                PATHS ${FIND_SFML_PATHS})

        # dynamic release library
        find_library(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_RELEASE
                NAMES ${FIND_SFML_COMPONENT_NAME}
                PATH_SUFFIXES lib64 lib
                PATHS ${FIND_SFML_PATHS})


        # dynamic debug library
        find_library(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_DEBUG
                NAMES ${FIND_SFML_COMPONENT_NAME}-d
                PATH_SUFFIXES lib64 lib
                PATHS ${FIND_SFML_PATHS})

        # choose the entries that fit the requested link type
        if(SFML_STATIC_LIBRARIES)
            if(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_RELEASE)
                set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_RELEASE})
            endif()
            if(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_DEBUG)
                set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_DEBUG})
            endif()
        else()
            if(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_RELEASE)
                set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_RELEASE})
            endif()
            if(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_DEBUG)
                set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_DEBUG})
            endif()
        endif()
    endif()

    if (SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG OR SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE)
        # library found
        set(SFML_${FIND_SFML_COMPONENT_UPPER}_FOUND TRUE)

        # if both are found, set SFML_XXX_LIBRARY to contain both
        if (SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG AND SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE)
            set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY debug     ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG}
                    optimized ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE})
        endif()

        # if only one debug/release variant is found, set the other to be equal to the found one
        if (SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG AND NOT SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE)
            # debug and not release
            set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG})
            set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY         ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG})
        endif()
        if (SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE AND NOT SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG)
            # release and not debug
            set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE})
            set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY       ${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE})
        endif()
    else()
        # library not found
        set(SFML_FOUND FALSE)
        set(SFML_${FIND_SFML_COMPONENT_UPPER}_FOUND FALSE)
        set(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY "")
        set(FIND_SFML_MISSING "${FIND_SFML_MISSING} SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY")
    endif()

    # mark as advanced
    MARK_AS_ADVANCED(SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY
            SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_RELEASE
            SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DEBUG
            SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_RELEASE
            SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_STATIC_DEBUG
            SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_RELEASE
            SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY_DYNAMIC_DEBUG)

    # add to the global list of libraries
    set(SFML_LIBRARIES ${SFML_LIBRARIES} "${SFML_${FIND_SFML_COMPONENT_UPPER}_LIBRARY}")
endforeach()

# handle success
if(SFML_FOUND AND NOT SFML_FIND_QUIETLY)
    message(STATUS "Found SFML ${SFML_VERSION_MAJOR}.${SFML_VERSION_MINOR}.${SFML_VERSION_PATCH} in ${SFML_INCLUDE_DIR}")
endif()
