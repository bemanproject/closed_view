// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_CLOSED_VIEW_CLOSED_HPP
#define BEMAN_CLOSED_VIEW_CLOSED_HPP

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
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

template <class R>
concept simple_view = // exposition only
    std::ranges::view<R> && std::ranges::range<const R> &&
    std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<const R>> &&
    std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;

// conditionally present iterator_category
template <typename I>
struct category_base {};
template <typename I>
    requires std::derived_from<typename std::iterator_traits<I>::iterator_category, std::forward_iterator_tag>
struct category_base<I> {
    using iterator_category = std::forward_iterator_tag;
};

template <typename I>
struct category_base_all {};
template <typename I>
    requires std::derived_from<typename std::iterator_traits<I>::iterator_category, std::forward_iterator_tag>
struct category_base_all<I> {
    using iterator_category = std::iterator_traits<I>::iterator_category;
};

template <typename T>
struct is_empty_view_t : std::false_type {};
template <typename T>
struct is_empty_view_t<std::ranges::empty_view<T>> : std::true_type {};
template <typename T>
inline constexpr bool is_empty_view = is_empty_view_t<T>::value;

#if __cpp_lib_ranges_repeat >= 202207L
template <typename T>
struct is_repeat_view_t : std::false_type {};
template <typename T, typename Bound>
struct is_repeat_view_t<std::ranges::repeat_view<T, Bound>> : std::true_type {};
template <typename T>
inline constexpr bool is_repeat_view = is_repeat_view_t<T>::value;
#endif

template <typename T>
struct empty_traits {
    static constexpr bool is_specialized = false;
};
template <typename T, std::size_t N>
struct empty_traits<std::span<T, N>> {
    static constexpr bool is_specialized = true;
    using specialization                 = std::span<T>;
};
template <typename CharT, typename Traits>
struct empty_traits<std::basic_string_view<CharT, Traits>> {
    static constexpr bool is_specialized = true;
    using specialization                 = std::basic_string_view<CharT, Traits>;
};
template <typename I, typename S, std::ranges::subrange_kind K>
struct empty_traits<std::ranges::subrange<I, S, K>> {
    static constexpr bool is_specialized = true;
    using specialization = std::ranges::subrange<std::ranges::iterator_t<std::ranges::subrange<I, S, K>>>;
};
template <typename W, typename Bound>
    requires std::ranges::random_access_range<std::ranges::iota_view<W, Bound>> &&
             std::ranges::sized_range<std::ranges::iota_view<W, Bound>>
struct empty_traits<std::ranges::iota_view<W, Bound>> {
    static constexpr bool is_specialized = true;
};

} // namespace detail

template <std::input_iterator I>
class lazy_counted_iterator : public detail::category_base<I> {
  public:
    constexpr auto& get_base_impl_only() const { return current; } // implementation only

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
    constexpr auto operator->() const noexcept
        requires std::contiguous_iterator<I>
    {
        return std::to_address(current);
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
        auto tmp = *this;
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
    requires std::convertible_to<decltype((F)), std::iter_difference_t<std::decay_t<decltype((E))>>>
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

namespace ranges {

template <std::ranges::view V>
class lazy_take_view : public std::ranges::view_interface<lazy_take_view<V>> {
  private:
    V                                  base_  = V(); // exposition only
    std::ranges::range_difference_t<V> count_ = 0;   // exposition only

    // [range.lazy.take.sentinel], class template lazy_take_view::sentinel
    template <bool>
    class sentinel; // exposition only

  public:
    lazy_take_view()
        requires std::default_initializable<V>
    = default;
    constexpr lazy_take_view(V base, std::ranges::range_difference_t<V> count)
        : base_(std::move(base)), count_(count) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin()
        requires(!detail::simple_view<V>)
    {
        if constexpr (std::ranges::sized_range<V>) {
            if constexpr (std::ranges::random_access_range<V>) {
                return std::ranges::begin(base_);
            } else {
                auto sz = range_difference_t<V>(size());
                return lazy_counted_iterator(std::ranges::begin(base_), sz);
            }
        } else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V>>) {
            auto it = std::ranges::begin(base_);
            auto sz = std::min(count_, std::ranges::end(base_) - it);
            return lazy_counted_iterator(std::move(it), sz);
        } else {
            return lazy_counted_iterator(std::ranges::begin(base_), count_);
        }
    }

    constexpr auto begin() const
        requires std::ranges::range<const V>
    {
        if constexpr (std::ranges::sized_range<const V>) {
            if constexpr (std::ranges::random_access_range<const V>) {
                return std::ranges::begin(base_);
            } else {
                auto sz = std::ranges::range_difference_t<const V>(size());
                return lazy_counted_iterator(std::ranges::begin(base_), sz);
            }
        } else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<const V>,
                                                     std::ranges::iterator_t<const V>>) {
            auto it = std::ranges::begin(base_);
            auto sz = std::min(count_, std::ranges::end(base_) - it);
            return lazy_counted_iterator(std::move(it), sz);
        } else {
            return lazy_counted_iterator(std::ranges::begin(base_), count_);
        }
    }

    constexpr auto end()
        requires(!detail::simple_view<V>)
    {
        if constexpr (std::ranges::sized_range<V>) {
            if constexpr (std::ranges::random_access_range<V>)
                return std::ranges::begin(base_) + std::ranges::range_difference_t<V>(size());
            else
                return std::default_sentinel;
        } else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V>>) {
            return std::default_sentinel;
        } else {
            return sentinel<false>{std::ranges::end(base_)};
        }
    }

    constexpr auto end() const
        requires std::ranges::range<const V>
    {
        if constexpr (std::ranges::sized_range<const V>) {
            if constexpr (std::ranges::random_access_range<const V>)
                return std::ranges::begin(base_) + std::ranges::range_difference_t<const V>(size());
            else
                return std::default_sentinel;
        } else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<const V>,
                                                     std::ranges::iterator_t<const V>>) {
            return std::default_sentinel;
        } else {
            return sentinel<true>{std::ranges::end(base_)};
        }
    }

    constexpr auto size()
        requires std::ranges::sized_range<V>
    {
        auto n = std::ranges::size(base_);
        return std::ranges::min(n, static_cast<decltype(n)>(count_));
    }

    constexpr auto size() const
        requires std::ranges::sized_range<const V>
    {
        auto n = std::ranges::size(base_);
        return std::ranges::min(n, static_cast<decltype(n)>(count_));
    }
};

template <class R>
lazy_take_view(R&&, std::ranges::range_difference_t<R>) -> lazy_take_view<std::views::all_t<R>>;

template <std::ranges::view V>
template <bool Const>
class lazy_take_view<V>::sentinel {
  private:
    using Base = detail::maybe_const<Const, V>; // exposition only
    template <bool OtherConst>
    using CI = lazy_counted_iterator<std::ranges::iterator_t<detail::maybe_const<OtherConst, V>>>; // exposition only
    std::ranges::sentinel_t<Base> end_ = std::ranges::sentinel_t<Base>();                          // exposition only

  public:
    sentinel() = default;
    constexpr explicit sentinel(std::ranges::sentinel_t<Base> end) : end_(end) {}
    constexpr sentinel(sentinel<!Const> s)
        requires Const && std::convertible_to<std::ranges::sentinel_t<V>, std::ranges::sentinel_t<Base>>
        : end_(std::move(s.end_)) {}

    constexpr std::ranges::sentinel_t<Base> base() const { return end_; }

    friend constexpr bool operator==(const CI<Const>& y, const sentinel& x) {
        return y.count() == 0 || y.get_base_impl_only() == x.end_;
    }

    template <bool OtherConst = !Const>
        requires std::sentinel_for<std::ranges::sentinel_t<Base>,
                                   std::ranges::iterator_t<detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const CI<OtherConst>& y, const sentinel& x) {
        return y.count() == 0 || y.get_base_impl_only() == x.end_;
    }
};

} // namespace ranges

namespace detail {

struct lazy_take_t {
    constexpr lazy_take_t() = default;
    constexpr auto operator()(std::ranges::input_range auto&& E, auto&& F) const
        requires std::convertible_to<decltype((F)), std::ranges::range_difference_t<decltype((E))>>
    {
        using T = std::remove_cvref_t<decltype((E))>;
        using D = std::ranges::range_difference_t<decltype((E))>;
        if constexpr (is_empty_view<T>)
            return E;
        else if constexpr (std::ranges::random_access_range<T> && std::ranges::sized_range<T> &&
                           empty_traits<T>::is_specialized) {
            if constexpr (requires { typename empty_traits<T>::specialization; })
                return empty_traits<T>::specialization(
                    std::ranges::begin(E), std::ranges::begin(E) + std::min<D>(std::ranges::distance(E), F));
            else
                return std::ranges::iota_view(*std::ranges::begin(E),
                                              *(std::ranges::begin(E) + std::min<D>(std::ranges::distance(E), F)));
        }
#if __cpp_lib_ranges_repeat >= 202207L
        else if constexpr (is_repeat_view<T>) {
            if constexpr (std::ranges::sized_range<T>)
                return std::views::repeat(*E.begin(), std::min<D>(std::ranges::distance(E), F));
            else
                return std::views::repeat(*E.begin(), static_cast<D>(F));
        }
#endif
        else
            return ranges::lazy_take_view(E, F);
    }

    constexpr auto operator()(auto&& E) const {
        return range_adaptor_closure_t(bind_back(*this, std::forward<decltype(E)>(E)));
    }
};

} // namespace detail

inline constexpr detail::lazy_take_t lazy_take{};

namespace ranges {

template <std::input_iterator I, std::sentinel_for<I> S>
class closed_view : public std::ranges::view_interface<closed_view<I, S>> {
  private:
    // [range.as.closed.iterator], class iota_view::iterator
    class iterator; // exposition only

    I start_ = I(); // exposition only
    S end_   = S(); // exposition only

  public:
    closed_view()
        requires std::default_initializable<I>
    = default;
    template <typename R>
        requires std::same_as<std::ranges::iterator_t<R>, I> && std::same_as<std::ranges::sentinel_t<R>, S>
    constexpr explicit closed_view(R&& r) : start_(std::ranges::begin(r)), end_(std::ranges::end(r)) {}
    constexpr closed_view(I start, S end) : start_(std::move(start)), end_(std::move(end)) {}

    constexpr iterator begin() const { return iterator{start_, end_}; }

    constexpr auto     end() const { return std::default_sentinel; }
    constexpr iterator end() const
        requires std::same_as<I, S>
    {
        return iterator{end_, end_, true};
    }

    constexpr auto size() const
        requires std::sized_sentinel_for<S, I>
    {
        return end_ - start_ + 1;
    }
};

template <std::input_iterator I, std::sentinel_for<I> S>
class closed_view<I, S>::iterator : detail::category_base_all<I> {
  private:
    I    current_ = I();   // exposition only
    S    last_    = S();   // exposition only
    bool is_end_  = false; // exposition only

    constexpr iterator(I current, S last, bool is_end)
        : current_(std::move(current)), last_(std::move(last)), is_end_(is_end) {}

  public:
    friend closed_view;

    using iterator_concept = std::conditional_t<
        std::random_access_iterator<I>,
        std::random_access_iterator_tag,
        std::conditional_t<
            std::bidirectional_iterator<I>,
            std::bidirectional_iterator_tag,
            std::conditional_t<std::forward_iterator<I>, std::forward_iterator_tag, std::input_iterator_tag>>>;
    using value_type      = std::iter_value_t<I>;
    using difference_type = std::iter_difference_t<I>;

    iterator()
        requires std::default_initializable<I>
    = default;
    constexpr iterator(I current, S last) : current_(std::move(current)), last_(std::move(last)) {}

    constexpr decltype(auto) operator*() const { return *current_; }
    constexpr decltype(auto) operator*() { return *current_; }
    constexpr iterator&      operator++() {
        if (current_ == last_)
            is_end_ = true;
        else
            ++current_;
        return *this;
    }
    constexpr void     operator++(int) { ++*this; }
    constexpr iterator operator++(int)
        requires std::forward_iterator<I>
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }
    constexpr iterator& operator--()
        requires std::bidirectional_iterator<I>
    {
        if (is_end_)
            is_end_ = false;
        else
            --current_;
        return *this;
    }
    constexpr iterator operator--(int)
        requires std::bidirectional_iterator<I>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type n)
        requires std::random_access_iterator<I>
    {
        current_ += n;
        return *this;
    }
    constexpr iterator& operator-=(difference_type n)
        requires std::random_access_iterator<I>
    {
        current_ -= n;
        return *this;
    }
    constexpr decltype(auto) operator[](difference_type n) const
        requires std::random_access_iterator<I>
    {
        return current_[n];
    }
    constexpr decltype(auto) operator[](difference_type n)
        requires std::random_access_iterator<I>
    {
        return current_[n];
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
        requires std::equality_comparable<I>
    {
        return x.current_ == y.current_ && x.last_ == y.last_ && x.is_end_ == y.is_end_;
    }
    friend constexpr bool operator==(const iterator& x, std::default_sentinel_t) noexcept { return x.is_end_; }
    friend constexpr bool operator<(const iterator& x, const iterator& y)
        requires std::random_access_iterator<I>
    {
        return x.current_ < y.current_ || (!x.is_end_ && y.is_end_);
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
        requires std::random_access_iterator<I>
    {
        return y < x;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
        requires std::random_access_iterator<I>
    {
        return !(y < x);
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
        requires std::random_access_iterator<I>
    {
        return !(x < y);
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
        requires std::random_access_iterator<I> && std::three_way_comparable<I>
    {
        if (x.is_end_ != y.is_end_)
            return x.is_end_ <=> y.is_end_;
        return x.current_ <=> y.current_;
    }

    friend constexpr iterator operator+(iterator i, difference_type n)
        requires std::random_access_iterator<I>
    {
        return iterator{i.current_ + n, i.last_, i.is_end_};
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
        requires std::random_access_iterator<I>
    {
        return iterator{i.current_ + n, i.last_, i.is_end_};
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
        requires std::random_access_iterator<I>
    {
        return iterator{i.current_ - n, i.last_, i.is_end_};
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        requires std::sized_sentinel_for<S, I>
    {
        return x.current_ - y.current_;
    }
    friend constexpr difference_type operator-(const iterator& x, std::default_sentinel_t)
        requires std::sized_sentinel_for<S, I>
    {
        return x.current_ - x.last_;
    }
    friend constexpr difference_type operator-(std::default_sentinel_t, const iterator& x)
        requires std::sized_sentinel_for<S, I>
    {
        return x.last_ - x.current_;
    }
};

template <class R>
closed_view(R&&) -> closed_view<std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>>;

} // namespace ranges

namespace detail {

struct as_closed_t : range_adaptor_closure<as_closed_t> {
    constexpr as_closed_t() = default;
    constexpr auto operator()(std::ranges::input_range auto&& E) const { return ranges::closed_view(E); }
};

} // namespace detail

inline constexpr detail::as_closed_t as_closed{};
inline constexpr auto                closed = [](auto&& E, auto&& F) { return ranges::closed_view(E, F); };
inline constexpr auto closed_iota = [](auto&& E, auto&& F) { return ranges::closed_view(std::views::iota(E, F)); };

} // namespace beman::closed_view

// Enable borrowed for lazy_take and closed_view
template <class T>
constexpr bool std::ranges::enable_borrowed_range<beman::closed_view::ranges::lazy_take_view<T>> =
    std::ranges::enable_borrowed_range<T>;
template <class I, class S>
constexpr bool std::ranges::enable_borrowed_range<beman::closed_view::ranges::closed_view<I, S>> = true;

#endif // BEMAN_CLOSED_VIEW_CLOSED_HPP
