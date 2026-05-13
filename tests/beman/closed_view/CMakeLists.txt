# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

find_package(GTest REQUIRED)

add_executable(beman.closed_view.tests.basic)
target_sources(beman.closed_view.tests.basic PRIVATE basic.test.cpp)
target_link_libraries(
    beman.closed_view.tests.basic
    PRIVATE beman::closed_view GTest::gtest GTest::gtest_main
)

if(BEMAN_CLOSED_VIEW_USE_MODULES)
    set_target_properties(
        beman.closed_view.tests.basic
        PROPERTIES CXX_MODULE_STD ON
    )
endif()

include(GoogleTest)
gtest_discover_tests(beman.closed_view.tests.basic DISCOVERY_TIMEOUT 60)
