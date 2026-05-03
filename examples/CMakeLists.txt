# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(ALL_EXAMPLES basic_example complex_example)
message("Examples to be built: ${ALL_EXAMPLES}")

foreach(example ${ALL_EXAMPLES})
    add_executable(beman.closed_view.examples.${example})
    target_sources(beman.closed_view.examples.${example} PRIVATE ${example}.cpp)
    target_link_libraries(
        beman.closed_view.examples.${example}
        PRIVATE beman::closed_view
    )
endforeach()
