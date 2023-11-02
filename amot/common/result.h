#pragma once

#include <exception>

namespace amot {

template<typename T>
struct Result {
	/**
	 * @brief 构造函数，初始化为默认值
	 */
	explicit Result() = default;

	/**
	 * @brief 正常返回时用结果初始化 Result
	 */
	explicit Result(T &&value) : _value(value) {}

	/**
	 * @brief 抛出异常时用异常初始化 Result
	 */
	explicit Result(std::exception_ptr &&exception_ptr) :_exception_ptr(exception_ptr) {}

	/**
	 * @brief 读取结果，有异常则抛出异常
	 */
	T get_or_throw() {
		if (_exception_ptr) {
			std::rethrow_exception(_exception_ptr);
		}
		return _value;
	}
private:
	T _value{};
	std::exception_ptr _exception_ptr;
};

template<>
struct Result<void> {
	/**
	 * @brief 构造函数，初始化为默认值
	 */
	explicit Result() = default;

	/**
	 * @brief 抛出异常时用异常初始化 Result
	 */
	explicit Result(std::exception_ptr &&exception_ptr) :_exception_ptr(exception_ptr) {}

	/**
	 * @brief 读取结果，有异常则抛出异常
	 */
	void get_or_throw() {
		if (_exception_ptr) {
			std::rethrow_exception(_exception_ptr);
		}
	}
private:
	std::exception_ptr _exception_ptr;
};
}