#pragma once

#include <memory>
#include <utility>

namespace amot {

template <class T = void>
struct NonVoidHelper {
    using Type = T;
};

// void特化，将无值类型转化为NonVoidHelper类型
template <>
struct NonVoidHelper<void> {
    using Type = NonVoidHelper;

    explicit NonVoidHelper() = default;

    template <class T>
    friend constexpr T &&operator,(T &&t, NonVoidHelper) {
        return std::forward<T>(t);
    }

    char const *repr() const noexcept {
        return "NonVoidHelper";
    }
};

template <class T>
struct Uninitialized {
    union {
        T m_value;
    };

    Uninitialized() noexcept {}

    Uninitialized(Uninitialized &&) = delete;

    ~Uninitialized() noexcept {}

    T move_value() {
        T ret(std::move(m_value));
        m_value.~T();
        return ret;
    }

    template <class... Ts>
    void put_value(Ts &&...args) {
        new (std::addressof(m_value)) T(std::forward<Ts>(args)...);
    }
};

template <>
struct Uninitialized<void> {
    auto move_value() {
        return NonVoidHelper<>{};
    }

    void put_value(NonVoidHelper<>) {}
};

// const类型特化
template <class T>
struct Uninitialized<T const> : Uninitialized<T> {};

// 引用类型特化
template <class T>
struct Uninitialized<T &> : Uninitialized<std::reference_wrapper<T>> {};

// 右值特化
template <class T>
struct Uninitialized<T &&> : Uninitialized<T> {};
} // namespace amot
