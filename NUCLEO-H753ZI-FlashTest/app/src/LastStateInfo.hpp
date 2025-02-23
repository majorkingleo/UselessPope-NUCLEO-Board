#pragma once

#include <chrono>
#include <os.hpp>
#include <mutex>
#include <optional>

template <class T> class LastStateInfo
{
public:
	struct Data
	{
		std::chrono::steady_clock::time_point when;
		T value;

		bool operator==( const Data & other ) const {
			return when == other.when && value == other.value;
		}

		bool operator!=( const Data & other ) const {
			return !operator==(other);
		}
	};

protected:
	std::optional<Data> data;
	mutable os::mutex m_lock;

public:
	virtual ~LastStateInfo() {}

	virtual void set( const T & data_ ) {
		auto lock = os::lock_guard(m_lock);
		data = Data{ std::chrono::steady_clock::now(), data_ };
	}

	virtual std::optional<Data> get() const {
		auto lock = os::lock_guard(m_lock);
		return data;
	}

};


