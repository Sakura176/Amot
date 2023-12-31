#pragma once

#include <coroutine>
#include <execution>
#include <list>
#include <spdlog/spdlog.h>

#include "awaiter.h"

namespace amot {
template <typename ValueType>
struct Channel {

	struct ChannelClosedException : std::exception {
		const char *what() const noexcept override {
			return "Channel is closed.";
		}
	};

	Channel(Channel &&channel) = delete;

	Channel(Channel &) = delete;

	Channel &operator=(Channel &) = delete;

	explicit Channel(int capacity = 0) : buffer_capacity(capacity) {
		_is_active.store(true, std::memory_order_relaxed);
	}

	~Channel() {
		close();
	}

	void check_closed() {
		if (!_is_active.load(std::memory_order_relaxed)) {
			throw ChannelClosedException();
		}
	}

	void close() {
		bool expect = true;
		if(_is_active.compare_exchange_strong(expect, false, std::memory_order_relaxed)) {
			clean_up();
		}
	}

	bool is_active() {
		return _is_active.load(std::memory_order_relaxed);
	}

	auto write(ValueType value) {
		check_closed();
		return WriterAwaiter<ValueType>(this, value);
	}

	auto operator<<(ValueType value) {
		return write(value);
	}

	auto read() {
		check_closed();
		return ReaderAwaiter<ValueType>(this);
	}

	auto operator>>(ValueType &value_ref) {
		auto awaiter = read();
		awaiter.p_value = &value_ref;
		return awaiter;
	}

	void remove_writer(WriterAwaiter<ValueType> *writer_awaiter) {
		std::lock_guard lock(channel_lock);
		auto size = writer_list.remove(writer_awaiter);
		spdlog::debug("remove writer ", size);
	}

	void remove_reader(ReaderAwaiter<ValueType> *reader_awaiter) {
		std::lock_guard lock(channel_lock);
		auto size = reader_list.remove(reader_awaiter);
		spdlog::debug("remove reader ", size);
	}

	void try_push_reader(ReaderAwaiter<ValueType> *reader_awaiter) {
		std::unique_lock lock(channel_lock);
		check_closed();

		if (!buffer.empty()) {
			auto value = buffer.front();
			buffer.pop();

			if (!writer_list.empty()) {
				auto writer = writer_list.front();
				writer_list.pop_front();
				buffer.push(writer->_value);
				lock.unlock();

				writer->resume();
			} else {
				lock.unlock();
			}

			reader_awaiter->resume(value);
			return;
		}

		if (!writer_list.empty()) {
			auto writer = writer_list.front();
			writer_list.pop_front();
			lock.unlock();

			reader_awaiter->resume(writer->_value);
			writer->resume();
			return;
		}
		reader_list.push_back(reader_awaiter);
	}

	void try_push_writer(WriterAwaiter<ValueType> *writer_awaiter) {
		std::unique_lock lock(channel_lock);
		check_closed();
		// suspended readers
		if (!reader_list.empty()) {
			auto reader = reader_list.front();
			reader_list.pop_front();
			lock.unlock();

			reader->resume(writer_awaiter->_value);
			writer_awaiter->resume();
			return;
		}
		// write to buffer
		if (buffer.size() < size_t(buffer_capacity)) {
			buffer.push(writer_awaiter->_value);
			lock.unlock();
			writer_awaiter->resume();
			return;
		}
		// suspend writer
		writer_list.push_back(writer_awaiter);
	}

private:
	void clean_up() {
		std::lock_guard lock(channel_lock);

		for (auto writer : writer_list) {
			writer->resume();
		}
		writer_list.clear();

		for (auto reader : reader_list) {
			reader->resume_unsafe();
		}
		reader_list.clear();

		decltype(buffer) empty_buffer;
		std::swap(buffer, empty_buffer);
	}

private:
	int buffer_capacity;
	std::queue<ValueType> buffer;
	std::list<WriterAwaiter<ValueType> *> writer_list;
	std::list<ReaderAwaiter<ValueType> *> reader_list;

	std::atomic<bool> _is_active;

	std::mutex channel_lock;
	std::condition_variable channel_condition;
};
}