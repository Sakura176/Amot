#pragma once

#include <system_error>
#include <type_traits>

namespace amot {
/*
 * @brief 模板类，用于表示可能出错的操作的结果。
 * @tparam T 操作的结果类型。
 * @note 该模板类包含一个成员变量：m_res，表示操作的结果。
 */
template <class T>
struct [[nodiscard]] expected {
    // use std::make_signed_t<T> to get the signed type of T
    std::make_signed_t<T> m_res;

    expected() = default;

    expected(std::make_signed_t<T> res) noexcept : m_res(res) {}

    // 返回错误代码，如果操作成功，则返回0。
    int error() const noexcept {
        if (m_res < 0) {
            return m_res;
        }
        return 0;
    }

    // 检查是否存在特定错误
    bool is_error(int err) const noexcept {
        return m_res == -err;
    }

    // 返回错误类型，如果操作成功，则返回空对象。
    std::error_code error_code() const noexcept {
        if (m_res < 0) {
            return std::error_code(-m_res, std::system_category());
        }
        return std::error_code();
    };

    // if error, throw std::system_error with error_code and what.
    T expect(char const *what) const {
        if (m_res < 0) {
            auto ec = error_code();
            throw std::system_error(ec, what);
        }
        return m_res;
    }

    T value() const {
        if (m_res < 0) {
            auto ec = error_code();
            throw std::system_error(ec);
        }
        return m_res;
    }

    T raw_value() const {
        return m_res;
    }
};

template <class U, class T>
expected<U> convert_error(T res) {
    if (res == -1) {
        return -errno;
    }
    return res;
}

template <int = 0, class T>
expected<T> convert_error(T res) {
    if (res == -1) {
        return -errno;
    }
    return res;
}

} // namespace amot
