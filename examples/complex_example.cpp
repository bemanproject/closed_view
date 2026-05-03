#include <algorithm>
#include <iostream>
#include <random>
#include <ranges>
#include <vector>

#if __cpp_lib_print >= 202207L
    #include <print>
#endif

#include <beman/closed_view/closed.hpp>

namespace ranges = std::ranges;
namespace views  = std::views;
namespace exe    = beman::closed_view;

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

int main() {
    std::random_device            rd;
    std::default_random_engine    eng{rd()};
    std::uniform_int_distribution distrib{0, 100};
    std::vector<int>              vec;
    for (int i = 1; i <= 100; ++i)
        vec.push_back(distrib(eng));
    std::cout << "Before: Element 5 - 25: ";
    println(exe::closed(&vec[4], &vec[24]));
    ranges::sort(exe::closed(vec.begin() + 9, vec.begin() + 19));
    std::cout << "After: Element 5 - 25: ";
    println(exe::closed(&vec[4], &vec[24]));
}
