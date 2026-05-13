# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.30...4.3)

include(infra/cmake/enable-experimental-import-std.cmake)

project(
    beman.closed_view
    DESCRIPTION "Range adaptors that adapts closed ranges into half-open ranges"
    LANGUAGES CXX
    VERSION 0.1.0
)

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_CLOSED_VIEW_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_CLOSED_VIEW_BUILD_EXAMPLES
    "Enable building examples. Default: ${PROJECT_IS_TOP_LEVEL}. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

option(
    BEMAN_CLOSED_VIEW_USE_MODULES
    "Provide beman.closed_view as a C++ module"
    OFF
)

if(BEMAN_CLOSED_VIEW_USE_MODULES)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/include/beman/closed_view/config_generated.hpp.in"
    "${PROJECT_BINARY_DIR}/include/beman/closed_view/config_generated.hpp"
    @ONLY
)

# for find of beman_install_library and configure_build_telemetry
include(infra/cmake/beman-install-library.cmake)
include(infra/cmake/BuildTelemetryConfig.cmake)

if(BEMAN_CLOSED_VIEW_USE_MODULES)
    add_library(beman.closed_view STATIC)
else()
    add_library(beman.closed_view INTERFACE)
endif()
add_library(beman::closed_view ALIAS beman.closed_view)

if(BEMAN_CLOSED_VIEW_USE_MODULES)
    target_sources(
        beman.closed_view
        PUBLIC
            FILE_SET CXX_MODULES
            FILE_SET HEADERS
                BASE_DIRS
                    "${CMAKE_CURRENT_SOURCE_DIR}/include"
                    "${CMAKE_CURRENT_BINARY_DIR}/include"
    )
    set_target_properties(beman.closed_view PROPERTIES CXX_MODULE_STD ON)
    target_compile_features(beman.closed_view PUBLIC cxx_std_23)
else()
    target_sources(
        beman.closed_view
        PUBLIC
            FILE_SET HEADERS
                BASE_DIRS
                    "${CMAKE_CURRENT_SOURCE_DIR}/include"
                    "${CMAKE_CURRENT_BINARY_DIR}/include"
    )
    set_target_properties(
        beman.closed_view
        PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${PROJECT_IS_TOP_LEVEL}
    )
endif()

add_subdirectory(include/beman/closed_view)

beman_install_library(beman.closed_view TARGETS beman.closed_view)
configure_build_telemetry()

if(BEMAN_CLOSED_VIEW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/beman/closed_view)
endif()

if(BEMAN_CLOSED_VIEW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
