cmake_minimum_required(VERSION 3.15)

# Enable the modern MSVC runtime selection behavior so that
# MSVC_RUNTIME_LIBRARY below actually takes effect.
cmake_policy(SET CMP0091 NEW)

project(mini_gms
    VERSION 1.0.0
    DESCRIPTION "mini_gms - a lightweight config language"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(MGMS_BUILD_SHARED "Build shared library" ON)
option(MGMS_BUILD_STATIC "Build static library" OFF)

set(MGMS_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

function(mgms_configure_target tgt)
    target_include_directories(${tgt} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    if(WIN32)
        set_target_properties(${tgt} PROPERTIES PREFIX "")
    endif()

    if(MSVC)
        set_property(TARGET ${tgt} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    set_target_properties(${tgt} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY         ${MGMS_OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${MGMS_OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${MGMS_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY         ${MGMS_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${MGMS_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MGMS_OUTPUT_DIR}
        ARCHIVE_OUTPUT_DIRECTORY         ${MGMS_OUTPUT_DIR}
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${MGMS_OUTPUT_DIR}
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${MGMS_OUTPUT_DIR}
    )
endfunction()

if(MGMS_BUILD_SHARED)
    add_library(mini_gms_shared SHARED
        src/mini_gms.cpp
        src/mini_gms_c.cpp
    )
    set_target_properties(mini_gms_shared PROPERTIES
        OUTPUT_NAME mini_gms
        VERSION   ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    if(MSVC)
        set_target_properties(mini_gms_shared PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()

    mgms_configure_target(mini_gms_shared)
endif()

if(MGMS_BUILD_STATIC)
    add_library(mini_gms_static STATIC
        src/mini_gms.cpp
        src/mini_gms_c.cpp
    )

    # Avoid clashing with the import library produced by the shared build
    # on Windows, where both would otherwise be named mini_gms.lib.
    if(WIN32 AND MGMS_BUILD_SHARED)
        set_target_properties(mini_gms_static PROPERTIES
            OUTPUT_NAME mini_gms_static)
    else()
        set_target_properties(mini_gms_static PROPERTIES
            OUTPUT_NAME mini_gms)
    endif()

    mgms_configure_target(mini_gms_static)
endif()