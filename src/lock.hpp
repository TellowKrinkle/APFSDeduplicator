#pragma once

#include "defer.hpp"
#include <os/lock.h>

namespace apfsdedup {

class UnfairLockGuard;

class UnfairLock {
	os_unfair_lock_s _lock;
public:
	UnfairLock(const UnfairLock& other) = delete;
	UnfairLock(UnfairLock&& other) = delete;

	inline UnfairLock(): _lock(OS_UNFAIR_LOCK_INIT) {}

	inline void lock() {
		os_unfair_lock_lock(&_lock);
	}

	inline void unlock() {
		os_unfair_lock_unlock(&_lock);
	}

	[[ nodiscard ]]
	inline bool trylock() {
		return os_unfair_lock_trylock(&_lock);
	}

	template<typename T>
	inline void withLock(T&& func) {
		this->lock();
		auto unlock = defer([this]() { this->unlock(); });
		func();
	}
};

} // namespace apfsdedup
