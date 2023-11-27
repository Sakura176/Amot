#pragma once

#include <coroutine>
#include <optional>
#include <list>

namespace amot {
template <typename T>
struct Generator {
	struct promise_type;
	using Handle = std::coroutine_handle<promise_type>;
	class ExhaustedException : std::exception {};
	struct promise_type {
		Generator<T> get_return_object() {
			return Generator{ Handle::from_promise(*this) };
		}

		std::suspend_always initial_suspend() noexcept { return {}; }

		std::suspend_always final_suspend() noexcept { return {}; }

		std::suspend_always yield_value(T value) noexcept {
			val = std::move(value);
			is_ready = true;
			return {};
		}

		void unhandled_exception() { throw; }

		bool is_ready = false;
		T val;
	};

	explicit Generator(Handle handle) : m_handle(handle) {}
	Generator(Generator &&generator) noexcept
		: m_handle(std::exchange(generator.handle, {})) {}

	Generator(Generator &) = delete;
	Generator &operator=(Generator &) = delete;

	~Generator() {
		if (m_handle)
			m_handle.destroy();
	}

	Generator static from_array(T array[], int n) {
		for (int i = 0; i < n; ++i) {
			co_yield array[i];
		}
	}

	Generator static from_array(std::list<std::string> array) {
		for (auto& elem : array) {
			co_yield elem;
		}
	}

	Generator static from(std::initializer_list<T> list) {
		for (auto& elem : list) {
			co_yield elem;
		}
	}

	template<typename ...TArgs>
	Generator static from(TArgs ...args) {
		(co_yield args, ...);
	}

	bool has_next() {
		if (m_handle.done()) {
			return false;
		}

		if (!m_handle.promise().is_ready) {
			m_handle.resume();
		}

		if (m_handle.done()) {
			return false;
		} else {
			return true;
		}
	}

	T next() {
		if (has_next()) {
			m_handle.promise().is_ready = false;
			return m_handle.promise().val;
		}
		throw ExhaustedException();
	}

private:
	Handle m_handle;
};
}