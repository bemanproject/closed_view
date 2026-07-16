# Examples for beman.closed_view

<!--
SPDX-License-Identifier: 2.0 license with LLVM exceptions
-->

List of usage examples for `beman.closed_view`.

## Samples

Check basic `beman.closed_view` library usages:

* local [./basic_example.cpp](./basic_example.cpp) or [basic_example@Compiler Explorer](https://godbolt.org/z/zdEKKG4cq)
* local [./complex_example.cpp](./complex_example.cpp) or [complex_example@Compiler Explorer](https://godbolt.org/z/hqh6zP7E9)

### Local Build and Run

```bash
# building
$ cmake --workflow --preset llvm-release

# run sample.cpp
$ ./build/llvm-release/examples/beman.closed_view.examples.basic_example
[0, 1, 2, 3, 4]
[0, 1, 2, 3, 4, 5]
[2147483627, 2147483628, 2147483629, 2147483630, 2147483631, 2147483632, 2147483633, 2147483634, 2147483635, 2147483636, 2147483637, 2147483638, 2147483639, 2147483640, 2147483641, 2147483642, 2147483643, 2147483644, 2147483645, 2147483646]
[2147483627, 2147483628, 2147483629, 2147483630, 2147483631, 2147483632, 2147483633, 2147483634, 2147483635, 2147483636, 2147483637, 2147483638, 2147483639, 2147483640, 2147483641, 2147483642, 2147483643, 2147483644, 2147483645, 2147483646, 2147483647]
[3, 4, 5]
[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
[0]
i = 2
[0]
i = 1
```
