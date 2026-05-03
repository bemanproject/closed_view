#include <sstream>
#include <print>
#include <ranges>

#if __cpp_lib_print >= 202207L
    #include <print>
#else
    #include <iostream>
#endif

#include <beman/closed_view/closed.hpp>

namespace views = std::views;
namespace exe   = beman::scan_view;

#if __cpp_lib_print >= 202207L && __cpp_lib_format_ranges >= 202207L
void print(auto&& rng) { std::print("{}", std::forward<decltype(rng)>(rng)); }

void println(auto&& rng) { std::println("{}", std::forward<decltype(rng)>(rng)); }
#else
void print(auto&& rng) {
    std::cout << "[";
    bool first = true;
    for (auto&& elem : rng) {
        if (first)
            first = false;
        else
            std::cout << ", ";
        std::cout << elem;
    }
    std::cout << "]";
}

void println(auto&& rng) {
    print(std::forward<decltype(rng)>(rng));
    std::cout << "\n";
}
#endif

// Example given in the paper for closed views. (Needs C++23)
int main() {
    println("{}", views::iota(0, 5)); // [0, 1, 2, 3, 4]
    println("{}", exe::closed(0, 5)); // [0, 1, 2, 3, 4, 5]

    int max = std::numeric_limits<int>::max();
    println("{}", views::iota(max - 20, max));      // fine but not including max
    println("{}", views::iota(max - 20, -max - 1)); // UB
    println("{}", exe::closed(max - 20, max));      // ok, includes max

    // A range with 11 elements but calculating 12th element overflows
    auto weird = views::iota(0) | views::filter([](auto i) { return i < 11; });
    println("{}", weird);                  // UB
    println("{}", weird | exe::as_closed); // fine, 0-10

    auto iss    = std::istringstream("0 1 2");
    auto weird2 = views::istream<int>(iss) | views::take(1);
    println("{}", weird2); // fine, [0]
    auto i = 0;
    iss >> i;
    // now i = 2 (!)

    auto iss2 = std::istringstream("0 1 2");
    auto ok2  = views::istream<int>(iss) | exe::lazy_take(1);
    println("{}", ok2); // fine, [0]
    auto i2 = 0;
    iss2 >> i2;
    // now i2 = 1 as expected

    return 0;
}
