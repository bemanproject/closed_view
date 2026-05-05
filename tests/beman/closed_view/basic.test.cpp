// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

#include <gtest/gtest.h>

#include <beman/closed_view/closed.hpp>

namespace exe = beman::closed_view;

struct NonConstView : std::ranges::view_base {
    explicit NonConstView(int* b, int* e) : b_(b), e_(e) {}
    const int* begin() { return b_; } // deliberately non-const
    const int* end() { return e_; }   // deliberately non-const
    const int* b_;
    const int* e_;
};

// Some basic examples of how transform_view might be used in the wild. This is a general
// collection of sample algorithms and functions that try to mock general usage of
// this view.

TEST(ClosedView, General) {
    std::vector<int> vec         = {1, 2, 3, 4};
    auto             transformed = exe::as_closed(vec.begin() + 1, vec.begin() + 3);
    ASSERT_EQ(std::ranges::size(transformed), 3);
    int expected[] = {2, 3, 4};
    ASSERT_TRUE(std::ranges::equal(transformed, expected));
    const auto& ct = transformed;
    ASSERT_TRUE(std::ranges::equal(ct, expected));

    auto transformed2 = exe::closed_iota(0, 10);
    ASSERT_EQ(std::ranges::size(transformed2), 11);
    int expected2[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ASSERT_TRUE(std::ranges::equal(transformed2, expected2));
    const auto& ct2 = transformed2;
    ASSERT_TRUE(std::ranges::equal(ct2, expected2));

    auto take1 = std::views::iota(0) | exe::lazy_take(5);
    ASSERT_EQ(std::ranges::distance(take1), 5);
    int expected3[] = {0, 1, 2, 3, 4};
    ASSERT_TRUE(std::ranges::equal(take1, expected3));
    const auto& ct3 = take1;
    ASSERT_TRUE(std::ranges::equal(ct3, expected3));

    auto take2 = exe::lazy_counted(std::views::iota(0).begin(), 10);
    ASSERT_EQ(std::ranges::size(take2), 10);
    int expected4[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    ASSERT_TRUE(std::ranges::equal(take2, expected4));
    const auto& ct4 = take2;
    ASSERT_TRUE(std::ranges::equal(ct4, expected4));
}

TEST(ClosedView, Iterator) {
    std::vector<int> vec         = {1, 2, 3, 4};
    auto             transformed = exe::as_closed(vec.begin() + 1, vec.begin() + 3);
    auto             it          = transformed.begin();
    ASSERT_EQ(*it, 2);
    ASSERT_EQ(it, transformed.begin());
    ASSERT_EQ(it - transformed.begin(), 0);
    ASSERT_EQ(it <=> transformed.begin(), std::strong_ordering::equal);
    ASSERT_TRUE(it < transformed.end());
    ++it;
    ASSERT_EQ(*it, 3);
    ASSERT_EQ(it - transformed.begin(), 1);
    ++it;
    ASSERT_EQ(*it, 4);
    ASSERT_EQ(it - transformed.begin(), 2);
    ASSERT_NE(it, transformed.end());
    ++it;
    ASSERT_EQ(it, transformed.end());
    ASSERT_TRUE(it > transformed.begin());
    ASSERT_EQ(it <=> transformed.begin(), std::strong_ordering::greater);
    ASSERT_EQ(it - transformed.begin(), 3);
    it -= 1;
    ASSERT_EQ(*it, 4);
    ASSERT_EQ(it - transformed.begin(), 2);
    it -= 2;
    ASSERT_EQ(*it, 2);
    ASSERT_EQ(it - transformed.begin(), 0);
    it += 3;
    ASSERT_EQ(it, transformed.end());
    ASSERT_EQ(it - transformed.begin(), 3);
}

TEST(ClosedView, Constexpr) {
    static constexpr std::array vec         = {1, 2, 3, 4};
    static constexpr auto       transformed = exe::as_closed(vec.begin() + 1, vec.begin() + 3);
    static_assert(std::ranges::distance(transformed.begin(), transformed.end()) == 3);
    static_assert(std::ranges::size(transformed) == 3);
    static constexpr std::array expected = {2, 3, 4};
    static_assert(std::ranges::equal(transformed, expected));

    static constexpr auto transformed2 = exe::closed_iota(0, 10);
    static_assert(std::ranges::distance(transformed2.begin(), transformed2.end()) == 11);
    static_assert(std::ranges::size(transformed2) == 11);
    static constexpr std::array expected2 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    static_assert(std::ranges::equal(transformed2, expected2));

    static constexpr auto take1 = std::views::iota(0) | exe::lazy_take(5);
    static_assert(std::ranges::distance(take1) == 5);
    static_assert(std::ranges::distance(take1.begin(), take1.end()) == 5);
    static constexpr std::array expected3 = {0, 1, 2, 3, 4};
    static_assert(std::ranges::equal(take1, expected3));

    static constexpr auto take2 = exe::lazy_counted(std::views::iota(0).begin(), 10);
    static_assert(std::ranges::size(take2) == 10);
    static_assert(std::ranges::distance(take2.begin(), take2.end()) == 10);
    static constexpr std::array expected4 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    static_assert(std::ranges::equal(take2, expected4));
}

TEST(ClosedView, Properties) {
    std::vector<int> vec         = {1, 2, 3, 4};
    auto             transformed = exe::as_closed(vec.begin() + 1, vec.begin() + 3);
    static_assert(std::is_same_v<std::ranges::range_value_t<decltype(transformed)>, int>);
    static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(transformed)>, int&>);
    static_assert(std::ranges::random_access_range<decltype(transformed)>);
    static_assert(std::ranges::common_range<decltype(transformed)>);
    static_assert(std::ranges::sized_range<decltype(transformed)>);
    static_assert(std::ranges::range<const decltype(transformed)>);
    static_assert(std::ranges::borrowed_range<decltype(transformed)>);
    ASSERT_EQ(transformed.size(), 3);

    auto transformed2 = exe::closed_iota(0, 10);
    static_assert(std::is_same_v<std::ranges::range_value_t<decltype(transformed2)>, int>);
    static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(transformed2)>, int>);
    static_assert(std::ranges::random_access_range<decltype(transformed2)>);
    static_assert(std::ranges::common_range<decltype(transformed2)>);
    static_assert(std::ranges::sized_range<decltype(transformed2)>);
    static_assert(std::ranges::range<const decltype(transformed2)>);
    static_assert(std::ranges::borrowed_range<decltype(transformed2)>);
    ASSERT_EQ(transformed2.size(), 11);

    auto take1 = std::views::iota(0) | exe::lazy_take(5);
    static_assert(std::is_same_v<std::ranges::range_value_t<decltype(take1)>, int>);
    static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(take1)>, int>);
    static_assert(std::ranges::forward_range<decltype(take1)> && !std::ranges::bidirectional_range<decltype(take1)>);
    static_assert(!std::ranges::common_range<decltype(take1)>);
    static_assert(!std::ranges::sized_range<decltype(take1)>);
    static_assert(std::ranges::range<const decltype(take1)>);
    static_assert(std::ranges::borrowed_range<decltype(take1)>);
    ASSERT_EQ(std::ranges::distance(take1.begin(), take1.end()), 5);

    auto take2 = exe::lazy_counted(std::views::iota(0).begin(), 10);
    static_assert(std::is_same_v<std::ranges::range_value_t<decltype(take2)>, int>);
    static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(take2)>, int>);
    static_assert(std::ranges::random_access_range<decltype(take2)>);
    static_assert(std::ranges::common_range<decltype(take2)>);
    static_assert(std::ranges::sized_range<decltype(take2)>);
    static_assert(std::ranges::range<const decltype(take2)>);
    static_assert(std::ranges::borrowed_range<decltype(take2)>);
    ASSERT_EQ(std::distance(take2.begin(), take2.end()), 10);
}

TEST(ClosedView, NonConstIterable) {
    // Test a view type that is not const-iterable.
    int  a[]         = {1, 2, 3, 4};
    auto transformed = NonConstView(a, a + 4) | exe::lazy_take(2);
    int  expected[]  = {1, 2};
    ASSERT_TRUE(std::ranges::equal(transformed, expected));

    auto transformed2 = NonConstView(a, a + 3) | exe::as_closed;
    int  expected2[]  = {1, 2, 3, 4};
    ASSERT_TRUE(std::ranges::equal(transformed2, expected2));
}

TEST(ClosedView, Borrowed) {
    std::vector<int>                         vec = {1, 2, 3, 4};
    decltype(exe::lazy_take(vec, 3).begin()) it;
    {
        auto transformed = exe::lazy_take(vec, 3);
        it               = transformed.begin();
    }
    ASSERT_EQ(*it, 1);
    ++it;
    ASSERT_EQ(*it, 2);
    ++it;
    ASSERT_EQ(*it, 3);

    decltype(exe::as_closed(vec).begin()) it2;
    {
        auto transformed = exe::as_closed(vec);
        it2              = transformed.begin();
    }
    ASSERT_EQ(*it2, 1);
    ++it2;
    ASSERT_EQ(*it2, 2);
    ++it2;
    ASSERT_EQ(*it2, 3);
    ++it2;
    ASSERT_EQ(*it2, 4);
}

#if __cplusplus >= 202302L && __cpp_lib_ranges >= 202207L // FTM for P2494R2
TEST(ClosedView, MoveOnly) {
    std::vector<std::unique_ptr<int>> vec4;
    vec4.push_back(std::make_unique<int>(5));
    vec4.push_back(std::make_unique<int>(2));
    vec4.push_back(std::make_unique<int>(10));
    auto out     = exe::lazy_take(vec4, 2) | std::views::transform([](const auto& p) { return *p; });
    int  check[] = {5, 2};
    ASSERT_TRUE(std::ranges::equal(out, check));

    auto out2 = exe::lazy_counted(vec4.begin(), 2) | std::views::transform([](const auto& p) { return *p; });
    ASSERT_TRUE(std::ranges::equal(out2, check));

    auto out3 =
        exe::as_closed(vec4.begin(), vec4.begin() + 2) | std::views::transform([](const auto& p) { return *p; });
    int check2[] = {5, 2, 10};
    ASSERT_TRUE(std::ranges::equal(out3, check2));
}
#endif
