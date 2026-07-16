// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_CLOSED_VIEW_CLOSED_HPP
#define BEMAN_CLOSED_VIEW_CLOSED_HPP

#include <beman/closed_view/config.hpp>

#if BEMAN_CLOSED_VIEW_USE_MODULES() && !defined(BEMAN_CLOSED_VIEW_INCLUDED_FROM_INTERFACE_UNIT)

import beman.closed_view;

#else

    #if !BEMAN_CLOSED_VIEW_USE_MODULES()

        #include <algorithm>
        #include <concepts>
        #include <functional>
        #include <span>
        #include <ranges>
        #include <type_traits>
        #include <utility>

    #endif // !BEMAN_CLOSED_VIEW_USE_MODULES()

    #include "detail/expo_only.hpp"

namespace beman::closed_view {

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
    requires std::convertible_to<decltype((F)), std::iter_difference_t<std::decay_t<decltype((E))> > >
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
class lazy_take_view : public std::ranges::view_interface<lazy_take_view<V> > {
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
        } else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V> >) {
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
                                                     std::ranges::iterator_t<const V> >) {
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
        } else if constexpr (std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V> >) {
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
                                                     std::ranges::iterator_t<const V> >) {
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

    #if __cpp_lib_ranges_reserve_hint >= 202502L
    constexpr auto reserve_hint() {
        if constexpr (std::ranges::approximately_sized_range<V>) {
            auto n = static_cast<std::ranges::range_difference_t<V> >(std::ranges::reserve_hint(base_));
            return detail::to_unsigned_like(std::ranges::min(n, count_));
        }
        return detail::to_unsigned_like(count_);
    }

    constexpr auto reserve_hint() const {
        if constexpr (std::ranges::approximately_sized_range<const V>) {
            auto n = static_cast<std::ranges::range_difference_t<const V> >(std::ranges::reserve_hint(base_));
            return detail::to_unsigned_like(std::ranges::min(n, count_));
        }
        return detail::to_unsigned_like(count_);
    }
    #endif
};

template <class R>
lazy_take_view(R&&, std::ranges::range_difference_t<R>) -> lazy_take_view<std::views::all_t<R> >;

template <std::ranges::view V>
template <bool Const>
class lazy_take_view<V>::sentinel {
  private:
    using Base = detail::maybe_const<Const, V>; // exposition only
    template <bool OtherConst>
    using CI = lazy_counted_iterator<std::ranges::iterator_t<detail::maybe_const<OtherConst, V> > >; // exposition only
    std::ranges::sentinel_t<Base> end_ = std::ranges::sentinel_t<Base>();                            // exposition only

  public:
    sentinel() = default;
    constexpr explicit sentinel(std::ranges::sentinel_t<Base> end) : end_(end) {}
    constexpr sentinel(sentinel<!Const> s)
        requires Const && std::convertible_to<std::ranges::sentinel_t<V>, std::ranges::sentinel_t<Base> >
        : end_(std::move(s.end_)) {}

    constexpr std::ranges::sentinel_t<Base> base() const { return end_; }

    friend constexpr bool operator==(const CI<Const>& y, const sentinel& x) {
        return y.count() == 0 || y.get_base_impl_only() == x.end_;
    }

    template <bool OtherConst = !Const>
        requires std::sentinel_for<std::ranges::sentinel_t<Base>,
                                   std::ranges::iterator_t<detail::maybe_const<OtherConst, V> > >
    friend constexpr bool operator==(const CI<OtherConst>& y, const sentinel& x) {
        return y.count() == 0 || y.get_base_impl_only() == x.end_;
    }
};

} // namespace ranges

namespace detail {

struct lazy_take_t {
    constexpr lazy_take_t() = default;
    constexpr auto operator()(std::ranges::input_range auto&& E, auto&& F) const
        requires std::convertible_to<decltype((F)), std::ranges::range_difference_t<decltype((E))> >
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
class as_closed_view : public std::ranges::view_interface<as_closed_view<I, S> > {
  private:
    // [range.as.closed.iterator], class iota_view::iterator
    class iterator; // exposition only

    I start_ = I(); // exposition only
    S end_   = S(); // exposition only

  public:
    as_closed_view()
        requires std::default_initializable<I>
    = default;
    template <std::ranges::range R>
        requires std::same_as<std::ranges::iterator_t<R>, I> && std::same_as<std::ranges::sentinel_t<R>, S>
    constexpr explicit as_closed_view(R&& r) : start_(std::ranges::begin(r)), end_(std::ranges::end(r)) {}
    constexpr as_closed_view(I start, S end) : start_(std::move(start)), end_(std::move(end)) {}

    constexpr iterator begin() const { return iterator{start_, end_}; }

    constexpr auto     end() const { return std::default_sentinel; }
    constexpr iterator end() const
        requires std::same_as<I, S>
    {
        return iterator{end_, end_, true};
    }

    [[nodiscard]] constexpr bool empty() const { return false; }
    constexpr auto               size() const
        requires std::sized_sentinel_for<S, I>
    {
        return end_ - start_ + 1;
    }
};

template <std::input_iterator I, std::sentinel_for<I> S>
class as_closed_view<I, S>::iterator : detail::category_base_all<I> {
  private:
    I    current_ = I();   // exposition only
    S    last_    = S();   // exposition only
    bool is_end_  = false; // exposition only

    constexpr iterator(I current, S last, bool is_end)
        : current_(std::move(current)), last_(std::move(last)), is_end_(is_end) {}

  public:
    friend as_closed_view;

    using iterator_concept = std::conditional_t<
        std::random_access_iterator<I>,
        std::random_access_iterator_tag,
        std::conditional_t<
            std::bidirectional_iterator<I>,
            std::bidirectional_iterator_tag,
            std::conditional_t<std::forward_iterator<I>, std::forward_iterator_tag, std::input_iterator_tag> > >;
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
        if (n >= 1 && current_ + (n - 1) == last_) {
            current_ += n - 1;
            is_end_ = true;
        } else
            current_ += n;
        return *this;
    }
    constexpr iterator& operator-=(difference_type n)
        requires std::random_access_iterator<I>
    {
        if (is_end_ && n >= 1) {
            is_end_ = false;
            --n;
        }
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
        return i += n;
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
        requires std::random_access_iterator<I>
    {
        return i += n;
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
        requires std::random_access_iterator<I>
    {
        return i -= n;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        requires std::sized_sentinel_for<S, I>
    {
        return x.current_ + x.is_end_ - y.current_ - y.is_end_;
    }
    friend constexpr difference_type operator-(const iterator& x, std::default_sentinel_t)
        requires std::sized_sentinel_for<S, I>
    {
        return x.current_ + x.is_end_ - x.last_;
    }
    friend constexpr difference_type operator-(std::default_sentinel_t, const iterator& x)
        requires std::sized_sentinel_for<S, I>
    {
        return x.last_ - x.current_ - x.is_end_;
    }
};

template <class R>
as_closed_view(R&&) -> as_closed_view<std::ranges::iterator_t<R>, std::ranges::sentinel_t<R> >;

} // namespace ranges

namespace detail {

struct as_closed_t : range_adaptor_closure<as_closed_t> {
    constexpr as_closed_t() = default;
    constexpr auto operator()(std::ranges::input_range auto&& E) const {
        return ranges::as_closed_view(std::forward<decltype(E)>(E));
    }
    constexpr auto operator()(auto&& E, auto&& F) const {
        return ranges::as_closed_view(std::forward<decltype(E)>(E), std::forward<decltype(F)>(F));
    }
};

} // namespace detail

inline constexpr detail::as_closed_t as_closed{};
inline constexpr auto closed_iota = [](auto&& E, auto&& F) { return ranges::as_closed_view(std::views::iota(E, F)); };

} // namespace beman::closed_view

// Enable borrowed for lazy_take and as_closed_view
template <class T>
constexpr bool std::ranges::enable_borrowed_range<beman::closed_view::ranges::lazy_take_view<T> > =
    std::ranges::enable_borrowed_range<T>;
template <class I, class S>
constexpr bool std::ranges::enable_borrowed_range<beman::closed_view::ranges::as_closed_view<I, S> > = true;

#endif // #if BEMAN_CLOSED_VIEW_USE_MODULES() &&
       // !defined(BEMAN_CLOSED_VIEW_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_CLOSED_VIEW_CLOSED_HPP
