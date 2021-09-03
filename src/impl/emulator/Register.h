// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::Impl::Emulator {
#define WREGISTER_BOOL                                      \
  {                                                         \
   public:                                                  \
    static void init() { state_ = false; }                  \
    static void write(const bool value) { state_ = value; } \
    static bool getCurrentState() { return state_; }        \
                                                            \
   private:                                                 \
    static bool state_;                                     \
  };
}  // namespace Clef::Impl::Emulator
