// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_CLOSED_VIEW_CLOSED_HPP
#define BEMAN_CLOSED_VIEW_CLOSED_HPP

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <span>
#include <ranges>
#include <type_traits>
#include <utility>

namespace beman::closed_view {

namespace detail {

// until C++23, `__movable_box` was named `__copyable_box` and required the stored type to be copy-constructible, not
// just move-constructible; we preserve the old behavior in pre-C++23 modes.
template <class Tp>
concept movable_box_object =
#if __cpp_lib_ranges >= 202207L
    std::move_constructible<Tp>
#else
    std::copy_constructible<Tp>
#endif
    && std::is_object_v<Tp>;

// Primary template - uses std::optional and introduces an empty state in case assignment fails.
template <movable_box_object Tp>
class movable_box {
    [[no_unique_address]] std::optional<Tp> val_;

  public:
    template <class... Args>
        requires std::is_constructible_v<Tp, Args...>
    constexpr explicit movable_box(std::in_place_t,
                                   Args&&... args) noexcept(std::is_nothrow_constructible_v<Tp, Args...>)
        : val_(std::in_place, std::forward<Args>(args)...) {}

    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<Tp>)
        requires std::default_initializable<Tp>
        : val_(std::in_place) {}

    movable_box(const movable_box&) = default;
    movable_box(movable_box&&)      = default;

    constexpr movable_box& operator=(const movable_box& other) noexcept(std::is_nothrow_copy_constructible_v<Tp>)
#if __cpp_lib_ranges >= 202207L
        requires std::copy_constructible<Tp>
#endif
    {
        if (this != std::addressof(other)) {
            if (other.has_value())
                val_.emplace(*other);
            else
                val_.reset();
        }
        return *this;
    }

    movable_box& operator=(movable_box&&)
        requires std::movable<Tp>
    = default;

    constexpr movable_box& operator=(movable_box&& other) noexcept(std::is_nothrow_move_constructible_v<Tp>) {
        if (this != std::addressof(other)) {
            if (other.has_value())
                val_.emplace(std::move(*other));
            else
                val_.reset();
        }
        return *this;
    }

    constexpr const Tp& operator*() const noexcept { return *val_; }
    constexpr Tp&       operator*() noexcept { return *val_; }

    constexpr const Tp* operator->() const noexcept { return val_.operator->(); }
    constexpr Tp*       operator->() noexcept { return val_.operator->(); }

    [[nodiscard]] constexpr bool has_value() const noexcept { return val_.has_value(); }
};

template <bool Const, class Tp>
using maybe_const = std::conditional_t<Const, const Tp, Tp>;

template <class Op, class Indices, class... BoundArgs>
struct perfect_forward_impl;

template <class Op, size_t... Idx, class... BoundArgs>
struct perfect_forward_impl<Op, std::index_sequence<Idx...>, BoundArgs...> {
  private:
    std::tuple<BoundArgs...> bound_args_;

  public:
    template <class... Args, class = std::enable_if_t<std::is_constructible_v<std::tuple<BoundArgs...>, Args&&...>>>
    explicit constexpr perfect_forward_impl(Args&&... bound_args) : bound_args_(std::forward<Args>(bound_args)...) {}

    perfect_forward_impl(const perfect_forward_impl&) = default;
    perfect_forward_impl(perfect_forward_impl&&)      = default;

    perfect_forward_impl& operator=(const perfect_forward_impl&) = default;
    perfect_forward_impl& operator=(perfect_forward_impl&&)      = default;

    template <class... Args, class = std::enable_if_t<std::is_invocable_v<Op, BoundArgs&..., Args...>>>
    constexpr auto
    operator()(Args&&... args) & noexcept(noexcept(Op()(std::get<Idx>(bound_args_)..., std::forward<Args>(args)...)))
        -> decltype(Op()(std::get<Idx>(bound_args_)..., std::forward<Args>(args)...)) {
        return Op()(std::get<Idx>(bound_args_)..., std::forward<Args>(args)...);
    }

    template <class... Args, class = std::enable_if_t<!std::is_invocable_v<Op, BoundArgs&..., Args...>>>
    auto operator()(Args&&...) & = delete;

    template <class... Args, class = std::enable_if_t<std::is_invocable_v<Op, const BoundArgs&..., Args...>>>
    constexpr auto operator()(Args&&... args) const& noexcept(noexcept(Op()(std::get<Idx>(bound_args_)...,
                                                                            std::forward<Args>(args)...)))
        -> decltype(Op()(std::get<Idx>(bound_args_)..., std::forward<Args>(args)...)) {
        return Op()(std::get<Idx>(bound_args_)..., std::forward<Args>(args)...);
    }

    template <class... Args, class = std::enable_if_t<!std::is_invocable_v<Op, const BoundArgs&..., Args...>>>
    auto operator()(Args&&...) const& = delete;

    template <class... Args, class = std::enable_if_t<std::is_invocable_v<Op, BoundArgs..., Args...>>>
    constexpr auto operator()(Args&&... args) && noexcept(noexcept(Op()(std::get<Idx>(std::move(bound_args_))...,
                                                                        std::forward<Args>(args)...)))
        -> decltype(Op()(std::get<Idx>(std::move(bound_args_))..., std::forward<Args>(args)...)) {
        return Op()(std::get<Idx>(std::move(bound_args_))..., std::forward<Args>(args)...);
    }

    template <class... Args, class = std::enable_if_t<!std::is_invocable_v<Op, BoundArgs..., Args...>>>
    auto operator()(Args&&...) && = delete;

    template <class... Args, class = std::enable_if_t<std::is_invocable_v<Op, const BoundArgs..., Args...>>>
    constexpr auto operator()(Args&&... args) const&& noexcept(noexcept(Op()(std::get<Idx>(std::move(bound_args_))...,
                                                                             std::forward<Args>(args)...)))
        -> decltype(Op()(std::get<Idx>(std::move(bound_args_))..., std::forward<Args>(args)...)) {
        return Op()(std::get<Idx>(std::move(bound_args_))..., std::forward<Args>(args)...);
    }

    template <class... Args, class = std::enable_if_t<!std::is_invocable_v<Op, const BoundArgs..., Args...>>>
    auto operator()(Args&&...) const&& = delete;
};

// perfect_forward implements a perfect-forwarding call wrapper as explained in [func.require].
template <class Op, class... Args>
using perfect_forward = perfect_forward_impl<Op, std::index_sequence_for<Args...>, Args...>;

struct compose_op {
    template <class Fn1, class Fn2, class... Args>
    constexpr auto operator()(Fn1&& f1, Fn2&& f2, Args&&... args) const
        noexcept(noexcept(std::invoke(std::forward<Fn1>(f1),
                                      std::invoke(std::forward<Fn2>(f2), std::forward<Args>(args)...))))
            -> decltype(std::invoke(std::forward<Fn1>(f1),
                                    std::invoke(std::forward<Fn2>(f2), std::forward<Args>(args)...))) {
        return std::invoke(std::forward<Fn1>(f1), std::invoke(std::forward<Fn2>(f2), std::forward<Args>(args)...));
    }
};

template <class Fn1, class Fn2>
struct compose_t : perfect_forward<compose_op, Fn1, Fn2> {
    using perfect_forward<compose_op, Fn1, Fn2>::perfect_forward;
};

template <class Fn1, class Fn2>
constexpr auto compose(Fn1&& f1, Fn2&& f2) noexcept(
    noexcept(compose_t<std::decay_t<Fn1>, std::decay_t<Fn2>>(std::forward<Fn1>(f1), std::forward<Fn2>(f2))))
    -> decltype(compose_t<std::decay_t<Fn1>, std::decay_t<Fn2>>(std::forward<Fn1>(f1), std::forward<Fn2>(f2))) {
    return compose_t<std::decay_t<Fn1>, std::decay_t<Fn2>>(std::forward<Fn1>(f1), std::forward<Fn2>(f2));
}

// CRTP base that one can derive from in order to be considered a range adaptor closure
// by the library. When deriving from this class, a pipe operator will be provided to
// make the following hold:
// - `x | f` is equivalent to `f(x)`
// - `f1 | f2` is an adaptor closure `g` such that `g(x)` is equivalent to `f2(f1(x))`
template <class Tp>
struct range_adaptor_closure;

// Type that wraps an arbitrary function object and makes it into a range adaptor closure,
// i.e. something that can be called via the `x | f` notation.
template <class Fn>
struct range_adaptor_closure_t : Fn, range_adaptor_closure<range_adaptor_closure_t<Fn>> {
    constexpr explicit range_adaptor_closure_t(Fn&& f) : Fn(std::move(f)) {}
};

template <class Tp>
concept RangeAdaptorClosure =
    std::derived_from<std::remove_cvref_t<Tp>, range_adaptor_closure<std::remove_cvref_t<Tp>>>;

template <class Tp>
struct range_adaptor_closure {
    template <std::ranges::viewable_range View, RangeAdaptorClosure Closure>
        requires std::same_as<Tp, std::remove_cvref_t<Closure>> && std::invocable<Closure, View>
    [[nodiscard]] friend constexpr decltype(auto)
    operator|(View&& view, Closure&& closure) noexcept(std::is_nothrow_invocable_v<Closure, View>) {
        return std::invoke(std::forward<Closure>(closure), std::forward<View>(view));
    }

    template <RangeAdaptorClosure Closure, RangeAdaptorClosure OtherClosure>
        requires std::same_as<Tp, std::remove_cvref_t<Closure>> &&
                 std::constructible_from<std::decay_t<Closure>, Closure> &&
                 std::constructible_from<std::decay_t<OtherClosure>, OtherClosure>
    [[nodiscard]] friend constexpr auto
    operator|(Closure&&      c1,
              OtherClosure&& c2) noexcept(std::is_nothrow_constructible_v<std::decay_t<Closure>, Closure> &&
                                          std::is_nothrow_constructible_v<std::decay_t<OtherClosure>, OtherClosure>) {
        return range_adaptor_closure_t(compose(std::forward<OtherClosure>(c2), std::forward<Closure>(c1)));
    }
};

template <size_t NBound, class = std::make_index_sequence<NBound>>
struct bind_back_op;

template <size_t NBound, size_t... Ip>
struct bind_back_op<NBound, std::index_sequence<Ip...>> {
    template <class Fn, class BoundArgs, class... Args>
    constexpr auto operator()(Fn&& f, BoundArgs&& bound_args, Args&&... args) const noexcept(noexcept(std::invoke(
        std::forward<Fn>(f), std::forward<Args>(args)..., std::get<Ip>(std::forward<BoundArgs>(bound_args))...)))
        -> decltype(std::invoke(std::forward<Fn>(f),
                                std::forward<Args>(args)...,
                                std::get<Ip>(std::forward<BoundArgs>(bound_args))...)) {
        return std::invoke(
            std::forward<Fn>(f), std::forward<Args>(args)..., std::get<Ip>(std::forward<BoundArgs>(bound_args))...);
    }
};

template <class Fn, class BoundArgs>
struct bind_back_t : perfect_forward<bind_back_op<std::tuple_size_v<BoundArgs>>, Fn, BoundArgs> {
    using perfect_forward<bind_back_op<std::tuple_size_v<BoundArgs>>, Fn, BoundArgs>::perfect_forward;
};

template <class Fn, class... Args>
    requires std::is_constructible_v<std::decay_t<Fn>, Fn> && std::is_move_constructible_v<std::decay_t<Fn>> &&
             (std::is_constructible_v<std::decay_t<Args>, Args> && ...) &&
             (std::is_move_constructible_v<std::decay_t<Args>> && ...)
constexpr auto
bind_back(Fn&& f, Args&&... args) noexcept(noexcept(bind_back_t<std::decay_t<Fn>, std::tuple<std::decay_t<Args>...>>(
    std::forward<Fn>(f), std::forward_as_tuple(std::forward<Args>(args)...))))
    -> decltype(bind_back_t<std::decay_t<Fn>, std::tuple<std::decay_t<Args>...>>(
        std::forward<Fn>(f), std::forward_as_tuple(std::forward<Args>(args)...))) {
    return bind_back_t<std::decay_t<Fn>, std::tuple<std::decay_t<Args>...>>(
        std::forward<Fn>(f), std::forward_as_tuple(std::forward<Args>(args)...));
}

template <class T>
using with_reference = T&; // exposition only
template <class T>
concept can_reference // exposition only
    = requires { typename with_reference<T>; };
template <class T>
concept dereferenceable // exposition only
    = requires(T& t) {
          { *t } -> can_reference; // not required to be equality-preserving
      };

// conditionally present iterator_category
template <typename I>
struct category_base {};
template <typename I>
    requires requires { typename std::iterator_traits<I>::iterator_category; }
struct category_base<I> {
    using iterator_category = std::conditional_t<
        std::derived_from<std::forward_iterator_tag, typename std::iterator_traits<I>::iterator_category>,
        std::forward_iterator_tag,
        typename std::iterator_traits<I>::iterator_category>;
};

} // namespace detail

template <std::input_iterator I>
class lazy_counted_iterator : public detail::category_base<I> {
  public:
    using iterator_type   = I;
    using value_type      = std::iter_value_t<I>;
    using difference_type = std::iter_difference_t<I>;
    using iterator_concept =
        std::conditional_t<std::forward_iterator<I>, std::forward_iterator_tag, std::input_iterator_tag>;
    constexpr lazy_counted_iterator()
        requires std::default_initializable<I>
    = default;
    constexpr lazy_counted_iterator(I i, std::iter_difference_t<I> n) : current(std::move(i)), length(n) {}
    template <class I2>
        requires std::convertible_to<const I2&, I>
    constexpr lazy_counted_iterator(const lazy_counted_iterator<I2>& x) : current(x.current), length(x.length) {}

    template <class I2>
        requires std::assignable_from<I&, const I2&>
    constexpr lazy_counted_iterator& operator=(const lazy_counted_iterator<I2>& x) {
        current = x.current;
        length  = x.length;
        return *this;
    }

    constexpr std::iter_difference_t<I> count() const noexcept { return length; }
    constexpr decltype(auto)            operator*() { return *current; }
    constexpr decltype(auto)            operator*() const
        requires detail::dereferenceable<const I>
    {
        return *current;
    }

    constexpr lazy_counted_iterator& operator++() {
        if (length > 1)
            ++current;
        --length;
        return *this;
    }
    constexpr void                  operator++(int) { ++*this; }
    constexpr lazy_counted_iterator operator++(int)
        requires std::forward_iterator<I>
    {
        lazy_counted_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    template <std::common_with<I> I2>
    friend constexpr std::iter_difference_t<I2> operator-(const lazy_counted_iterator&     x,
                                                          const lazy_counted_iterator<I2>& y) {
        return y.length - x.length;
    }
    friend constexpr std::iter_difference_t<I> operator-(const lazy_counted_iterator& x, std::default_sentinel_t) {
        return -x.length;
    }
    friend constexpr std::iter_difference_t<I> operator-(std::default_sentinel_t, const lazy_counted_iterator& y) {
        return y.length;
    }

    template <std::common_with<I> I2>
    friend constexpr bool operator==(const lazy_counted_iterator& x, const lazy_counted_iterator<I2>& y) {
        return x.length == y.length;
    }
    friend constexpr bool operator==(const lazy_counted_iterator& x, std::default_sentinel_t) { return x.length == 0; }

    template <std::common_with<I> I2>
    friend constexpr std::strong_ordering operator<=>(const lazy_counted_iterator&     x,
                                                      const lazy_counted_iterator<I2>& y) {
        return y.length <=> x.length;
    }

    friend constexpr std::iter_rvalue_reference_t<I>
    iter_move(const lazy_counted_iterator& i) noexcept(noexcept(std::ranges::iter_move(i.current))) {
        return std::ranges::iter_move(i.current);
    }
    template <std::indirectly_swappable<I> I2>
    friend constexpr void
    iter_swap(const lazy_counted_iterator&     x,
              const lazy_counted_iterator<I2>& y) noexcept(noexcept(std::ranges::iter_swap(x.current, y.current))) {
        std::ranges::iter_swap(x.current, y.current);
    }

  private:
    I                         current = I(); // exposition only
    std::iter_difference_t<I> length  = 0;   // exposition only
};

inline constexpr auto lazy_counted = [](auto&& E, auto&& F)
    requires std::convertible_to<std::iter_difference_t<std::decay_t<decltype((E))>>, decltype((F))>
{
    using T = std::decay_t<decltype((E))>;
    using D = std::iter_difference_t<T>;
    if constexpr (std::contiguous_iterator<T>)
        return std::span(std::to_address(E), static_cast<std::size_t>(static_cast<D>(F)));
    else if constexpr (std::random_access_iterator<T>)
        return std::ranges::subrange(E, E + static_cast<D>(F));
    else
        return std::ranges::subrange(lazy_counted_iterator(E, F), std::default_sentinel);
};

} // namespace beman::closed_view

#endif // BEMAN_CLOSED_VIEW_CLOSED_HPP
