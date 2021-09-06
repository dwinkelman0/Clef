// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <if/Interrupts.h>

#include <mutex>

namespace Clef::If {
std::mutex globalMutex_;
std::unique_lock<std::mutex> globalLock_(globalMutex_, std::defer_lock);

bool areInterruptsEnabled() { return !globalLock_.owns_lock(); }

void disableInterrupts() { globalLock_.lock(); }

void enableInterrupts() { globalLock_.unlock(); }
}  // namespace Clef::If
