#pragma once

#include <atomic>

template<class T>
class AtomicPointer
{
public:
	using value_type = T;
	using pointer_type = T*;
	using const_pointer_type = const T*;

private:
	std::atomic<T*> value = nullptr;
public:

	AtomicPointer() = default;

	AtomicPointer( T *t )
	{
		value = t;
	}

	operator pointer_type() {
		T* t  = value.load();
		return t;
	}

	pointer_type operator->() {
		T *t = value.load();
		return t;
	}

	void operator=( T* t ) {
		value = t;
	}

	operator bool() const {
		return value.load() != nullptr;
	}

	pointer_type get() {
		T* t  = value.load();
		return t;
	}

	const_pointer_type get() const {
		const_pointer_type t  = value.load();
		return t;
	}
};
