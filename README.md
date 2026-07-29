# beman.closed_view: Range adaptors that adapts closed ranges into half-open ranges

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

<!-- markdownlint-disable line-length -->
[![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#the-beman-library-maturity-model)
[![Continuous Integration Tests](https://github.com/bemanproject/closed_view/actions/workflows/ci_tests.yml/badge.svg)](https://github.com/bemanproject/closed_view/actions/workflows/ci_tests.yml)
[![Lint Check (pre-commit)](https://github.com/bemanproject/closed_view/actions/workflows/pre-commit-check.yml/badge.svg)](https://github.com/bemanproject/closed_view/actions/workflows/pre-commit-check.yml)
[![Coverage](https://coveralls.io/repos/github/bemanproject/closed_view/badge.svg?branch=main)](https://coveralls.io/github/bemanproject/closed_view?branch=main)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)
[![Compiler Explorer Example](https://img.shields.io/badge/Try%20it%20on%20Compiler%20Explorer-grey?logo=compilerexplorer&logoColor=67c52a)](https://godbolt.org/z/zdEKKG4cq)
<!-- markdownlint-restore -->

**Implements**: Range adaptor and iterators proposed in [Adaptors For Closed Ranges (P4211R0)](https://wg21.link/P4211R0).

**Difference from the paper**:

- Implemented `reserve_hint` are hidden behind a feature-test macro.

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

`beman.closed_view` is licensed under the Apache License v2.0 with LLVM Exceptions.

## Usage

The proposal will add several facilities that accommodates (both side)
closed ranges and adapts them into half-open ranges that every other
C++ range facilities expects. Specifically, the proposal introduced
the following additions:

- `views::closed` and `ranges::closed_view`: Adapting a pair of
  iterators and sentinels representing closed ranges into
  half-open ones.
- `views::lazy_counted` and `std::lazy_counted_iterator`: A version of
  `counted_iterator` that skips the last increment of underlying
  iterators.
- `views::lazy_take` and `ranges::lazy_take_view`: A version of
  `take_view` that utilize lazy version of `counted_iterator` to prevent
  overshooting the underlying range.
- `views::closed_iota(a, b)` that represents the range `[a, b]`
  (closed version of `views::iota`).
  Equivalent to `views::closed` + `views::iota(a, b)`.

```cpp
#include <limits>
#include <sstream>
#include <print>
#include <ranges>
#include <vector>

#include <beman/closed_view/closed.hpp>

namespace views = std::views;
namespace exe = beman::closed_view;

// Example given in the paper for closed views. (Needs C++23)
int main()
{
    std::println("{}", views::iota(0, 5)); // [0, 1, 2, 3, 4]
    std::println("{}", exe::closed_iota(0, 5)); // [0, 1, 2, 3, 4, 5]

    int max = std::numeric_limits<int>::max();
    std::println("{}", views::iota(max - 20, max)); // fine but not including max
    std::println("{}", views::iota(max - 20, -max - 1)); // UB
    std::println("{}", exe::closed_iota(max - 20, max)); // ok, includes max

    std::vector vec{1, 2, 3, 4, 5};
    std::println("{}", exe::as_closed(vec.begin() + 2, vec.begin() + 4)); // [3, 4, 5]

    // A range with 11 elements but calculating 12th element overflows
    auto weird = views::iota(0) | views::filter([](auto i) { return i < 11; });
    std::println("{}", weird); // UB
    std::println("{}", weird | exe::lazy_take(11)); // fine, 0-10
    std::println("{}", weird | views::take(10) | exe::as_closed); // also fine, 0-10

    auto iss = std::istringstream("0 1 2");
    auto weird2 = views::istream<int>(iss) | views::take(1);
    std::println("{}", weird2); // fine, [0]
    auto i = 0; iss >> i;
    std::println("{}", i); // now i = 2 (!)

    auto iss2 = std::istringstream("0 1 2");
    auto ok2 = views::istream<int>(iss2) | exe::lazy_take(1);
    std::println("{}", ok2); // fine, [0]
    auto i2 = 0; iss2 >> i2;
    std::println("{}", i2); // now i2 = 1 as expected

    return 0;
}
```

Full runnable examples can be found in [`examples/`](examples/).

## Dependencies

### Build Environment

This project requires at least the following to build:

* A C++ compiler that conforms to the C++20 standard or greater
* CMake 3.30 or later
* (Test Only) GoogleTest

You can disable building tests by setting CMake option `BEMAN_CLOSED_VIEW_BUILD_TESTS` to
`OFF` when configuring the project.

You can disable building examples by setting CMake option `BEMAN_CLOSED_VIEW_BUILD_EXAMPLES` to
`OFF` when configuring the project.

### Supported Platforms

| Compiler   | Version | C++ Standards | Standard Library  |
|------------|---------|---------------|-------------------|
| GCC        | 16-13   | C++26-C++20   | libstdc++         |
| GCC        | 12-11   | C++23, C++20  | libstdc++         |
| Clang      | 20-19   | C++26-C++20   | libstdc++, libc++ |
| Clang      | 18-17   | C++26-C++20   | libc++            |
| Clang      | 18-17   | C++20         | libstdc++         |
| AppleClang | latest  | C++26-C++20   | libc++            |
| MSVC       | latest  | C++23         | MSVC STL          |

- **Note**: module support is only available in Clang for now; GCC 16 somehow ICEs when compiling with modules.

## Development

See the [Contributing Guidelines](CONTRIBUTING.md).

## Integrate beman.closed_view into your project

### Build

You can build closed_view using a CMake workflow preset:

```bash
cmake --workflow --preset gcc-release
```

To list available workflow presets, you can invoke:

```bash
cmake --list-presets=workflow
```

For details on building beman.closed_view without using a CMake preset, refer to the
[Contributing Guidelines](CONTRIBUTING.md).

### Installation

#### Vcpkg

The preferred way to install closed_view is via vcpkg. To do so, after installing vcpkg
itself, you need to add support for the Beman project's [vcpkg
registry](https://github.com/bemanproject/vcpkg-registry) by configuring a
`vcpkg-configuration.json` file (which closed_view [provides](vcpkg-configuration.json)).

Then, simply run `vcpkg install beman-closed-view`.

#### Manual

To install beman.closed_view globally after building with the `gcc-release` preset, you can
run:

```bash
sudo cmake --install build/gcc-release
```

Alternatively, to install to a prefix, for example `/opt/beman`, you can run:

```bash
sudo cmake --install build/gcc-release --prefix /opt/beman
```

This will generate the following directory structure:

```txt
/opt/beman
├── include
│   └── beman
│       └── closed_view
│           ├── closed.hpp
│           └── ...
└── lib
    └── cmake
        └── beman.closed_view
            ├── beman.closed_view-config-version.cmake
            ├── beman.closed_view-config.cmake
            └── beman.closed_view-targets.cmake
```

### CMake Configuration

If you installed beman.closed_view to a prefix, you can specify that prefix to your CMake
project using `CMAKE_PREFIX_PATH`; for example, `-DCMAKE_PREFIX_PATH=/opt/beman`.

You need to bring in the `beman.closed_view` package to define the `beman::closed_view` CMake
target:

```cmake
find_package(beman.closed_view REQUIRED)
```

You will then need to add `beman::closed_view` to the link libraries of any libraries or
executables that include `beman.closed_view` headers.

```cmake
target_link_libraries(yourlib PUBLIC beman::closed_view)
```

### Using beman.closed_view

To use `beman.closed_view` in your C++ project,
include an appropriate `beman.closed_view` header from your source code.

```c++
#include <beman/closed_view/closed.hpp>
```
