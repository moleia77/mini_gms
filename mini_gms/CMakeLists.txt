cmake_minimum_required(VERSION 3.15)

# Enable the modern MSVC runtime selection behavior so that
# MSVC_RUNTIME_LIBRARY below actually takes effect.
cmake_policy(SET CMP0091 NEW)

project(mini_gms
    VERSION 1.1.0
    DESCRIPTION "mini_gms - a lightweight config language"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Options ----
option(MGMS_BUILD_SHARED      "Build shared library"                 ON)
option(MGMS_BUILD_STATIC      "Build static library"                 OFF)
option(MGMS_BUILD_TESTS       "Build the mini_gms test executable"   ON)
option(MGMS_COPY_DLL_TO_PYTHON "Copy the built dll into python/mini_gms for local pip install" OFF)

# Build artifacts go under the build tree, never into the source tree.
set(MGMS_OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib)

# All source files that make up the library. The old 1.0 sources under
# old_file/ are archived and intentionally not listed here.
set(MGMS_SOURCES
    src/mini_gms_doc.cpp
    src/mini_gms_parser.cpp
    src/mini_gms_c.cpp
)

function(mgms_configure_target tgt)
    target_include_directories(${tgt} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
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

# ---- Shared library ----
if(MGMS_BUILD_SHARED)
    add_library(mini_gms_shared SHARED ${MGMS_SOURCES})
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

    # Optional convenience step: copy the built dll next to the Python
    # package so that `pip install .` can bundle it. Off by default so
    # that a normal build never writes into the source tree.
    if(WIN32 AND MGMS_COPY_DLL_TO_PYTHON)
        add_custom_command(TARGET mini_gms_shared POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:mini_gms_shared>
                ${CMAKE_CURRENT_SOURCE_DIR}/python/mini_gms/mini_gms.dll
            COMMENT "Copying mini_gms.dll to python/mini_gms for local pip install"
        )
    endif()
endif()

# ---- Static library ----
if(MGMS_BUILD_STATIC)
    add_library(mini_gms_static STATIC ${MGMS_SOURCES})

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

# ---- Tests ----
if(MGMS_BUILD_TESTS)
    add_executable(mini_gms_test
        test/mini_gms_test.cpp
        ${MGMS_SOURCES}
    )
    target_include_directories(mini_gms_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
    if(MSVC)
        set_property(TARGET mini_gms_test PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set_target_properties(mini_gms_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY         ${MGMS_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${MGMS_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MGMS_OUTPUT_DIR}
    )

    enable_testing()
    add_test(NAME mini_gms_test COMMAND mini_gms_test)
endif()