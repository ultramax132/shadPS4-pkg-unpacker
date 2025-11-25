# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(pkg_unpacker CXX)

# Source files from the main project
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Find required packages
find_package(ZLIB 1.3 MODULE REQUIRED)

# Try to find fmt from system, otherwise use from externals
find_package(fmt 10.2.0 CONFIG QUIET)
if (NOT fmt_FOUND)
    # Use fmt from externals
    add_subdirectory(${PROJECT_ROOT}/externals/fmt fmt)
    set(fmt_TARGET fmt::fmt)
else()
    set(fmt_TARGET fmt::fmt)
endif()

# Add toml11 from externals (needed for config)
if (NOT TARGET toml11::toml11)
    add_subdirectory(${PROJECT_ROOT}/externals/toml11 toml11)
endif()

# Add tracy stub (needed for debug.h but we can make it optional)
# For standalone tool, we'll create a minimal tracy stub
add_library(tracy_stub INTERFACE)
target_include_directories(tracy_stub INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_ROOT}/externals/tracy/public
)
# Create a tracy directory structure for the include
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tracy)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tracy_stub.hpp ${CMAKE_CURRENT_BINARY_DIR}/tracy/Tracy.hpp COPYONLY)
target_include_directories(tracy_stub INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_library(Tracy::TracyClient ALIAS tracy_stub)

# Find cryptopp - try system first, then externals
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT MSVC)
    find_package(cryptopp 8.9.0 MODULE QUIET)
    if (NOT cryptopp_FOUND)
        # Use cryptopp from externals - need to set up cryptopp-cmake
        set(CRYPTOPP_INSTALL OFF)
        set(CRYPTOPP_BUILD_TESTING OFF)
        set(CRYPTOPP_SOURCES ${PROJECT_ROOT}/externals/cryptopp)
        set(CRYPTOPP_DISABLE_AESNI ON)
        set(CRYPTOPP_DISABLE_AVX2 ON)
        add_subdirectory(${PROJECT_ROOT}/externals/cryptopp-cmake ${CMAKE_CURRENT_BINARY_DIR}/cryptopp-cmake)
        file(COPY ${PROJECT_ROOT}/externals/cryptopp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/cryptopp FILES_MATCHING PATTERN "*.h")
        set_target_properties(cryptopp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/cryptopp")
        set(cryptopp_TARGET cryptopp)
    else()
        set(cryptopp_TARGET cryptopp::cryptopp)
    endif()
endif()

# Add executable
add_executable(pkg_unpacker
    main.cpp
    trp_stub.cpp
    scm_rev.cpp
    ${PROJECT_ROOT}/src/core/file_format/pkg.cpp
    ${PROJECT_ROOT}/src/core/file_format/pkg.h
    ${PROJECT_ROOT}/src/core/file_format/pkg_type.cpp
    ${PROJECT_ROOT}/src/core/file_format/pkg_type.h
    ${PROJECT_ROOT}/src/core/file_format/pfs.h
    ${PROJECT_ROOT}/src/core/crypto/crypto.cpp
    ${PROJECT_ROOT}/src/core/crypto/crypto.h
    ${PROJECT_ROOT}/src/core/crypto/keys.h
    ${PROJECT_ROOT}/src/common/io_file.cpp
    ${PROJECT_ROOT}/src/common/io_file.h
    ${PROJECT_ROOT}/src/common/endian.h
    ${PROJECT_ROOT}/src/common/types.h
    ${PROJECT_ROOT}/src/common/error.cpp
    ${PROJECT_ROOT}/src/common/error.h
    ${PROJECT_ROOT}/src/common/enum.h
    ${PROJECT_ROOT}/src/common/concepts.h
    ${PROJECT_ROOT}/src/common/logging/formatter.h
    ${PROJECT_ROOT}/src/common/path_util.cpp
    ${PROJECT_ROOT}/src/common/path_util.h
    ${PROJECT_ROOT}/src/common/logging/backend.cpp
    ${PROJECT_ROOT}/src/common/logging/backend.h
    ${PROJECT_ROOT}/src/common/logging/log.h
    ${PROJECT_ROOT}/src/common/logging/log_entry.h
    ${PROJECT_ROOT}/src/common/logging/text_formatter.cpp
    ${PROJECT_ROOT}/src/common/logging/text_formatter.h
    ${PROJECT_ROOT}/src/common/logging/types.h
    ${PROJECT_ROOT}/src/common/logging/filter.cpp
    ${PROJECT_ROOT}/src/common/logging/filter.h
    ${PROJECT_ROOT}/src/common/assert.cpp
    ${PROJECT_ROOT}/src/common/assert.h
    ${PROJECT_ROOT}/src/common/config.cpp
    ${PROJECT_ROOT}/src/common/config.h
    ${PROJECT_ROOT}/src/common/scm_rev.h
    ${PROJECT_ROOT}/src/common/string_util.cpp
    ${PROJECT_ROOT}/src/common/string_util.h
    ${PROJECT_ROOT}/src/common/thread.cpp
    ${PROJECT_ROOT}/src/common/thread.h
    ${PROJECT_ROOT}/src/common/scope_exit.h
    ${PROJECT_ROOT}/src/common/debug.h
    ${PROJECT_ROOT}/src/common/bounded_threadsafe_queue.h
)

# Include directories
target_include_directories(pkg_unpacker PRIVATE 
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/src
)

# Link libraries
target_link_libraries(pkg_unpacker PRIVATE 
    ZLIB::ZLIB 
    ${fmt_TARGET}
    toml11::toml11
    Tracy::TracyClient
)

# Link cryptopp if available
if (DEFINED cryptopp_TARGET)
    target_link_libraries(pkg_unpacker PRIVATE ${cryptopp_TARGET})
elseif (TARGET cryptopp::cryptopp)
    target_link_libraries(pkg_unpacker PRIVATE cryptopp::cryptopp)
elseif (TARGET cryptopp)
    target_link_libraries(pkg_unpacker PRIVATE cryptopp)
endif()

# Set output directory
set_target_properties(pkg_unpacker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

