# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if(BEMAN_CLOSED_VIEW_USE_MODULES)
    target_sources(
        beman.closed_view
        PUBLIC
            FILE_SET CXX_MODULES FILES closed_view.cppm
            FILE_SET HEADERS
                FILES
                    config.hpp
                    closed.hpp
                    detail/expo_only.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/closed_view/config_generated.hpp"
    )
else()
    target_sources(
        beman.closed_view
        PUBLIC
            FILE_SET HEADERS
                FILES
                    config.hpp
                    closed.hpp
                    detail/expo_only.hpp
                    "${PROJECT_BINARY_DIR}/include/beman/closed_view/config_generated.hpp"
    )
endif()
