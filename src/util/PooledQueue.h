// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <assert.h>
#include <stdint.h>

/**
 * A queue implemented as a ring buffer. The underlying memory is statically
 * allocated. It can contain a maximum of N - 1 elements. When firstIndex_ ==
 * lastIndex_, the queue is considered to be empty, so it is not possible for
 * all N elements to be simultaneously occupied.
 */
namespace Clef::Util {
template <typename T, uint16_t N>
class PooledQueue {
 public:
  class Iterator {
    friend class PooledQueue;

   private:
    Iterator(const PooledQueue *queue, const uint16_t index)
        : queue_(queue), index_(index) {
      assert(index_ < N);
      assertInBounds();
    }

   public:
    inline explicit operator bool() const {
      assertInBounds();
      return queue_;
    }
    inline T *operator->() const {
      assert(queue_);
      assertInBounds();
      return const_cast<T *>(queue_->data_) + index_;
    }
    inline T &operator*() const {
      assert(queue_);
      assertInBounds();
      return const_cast<T *>(queue_->data_)[index_];
    }
    inline bool operator==(const Iterator &other) const {
      assert(queue_);
      assertInBounds();
      return queue_ == other.queue_ && index_ == other.index_;
    }

    /**
     * Get the next iterator in the queue.
     */
    inline Iterator next() const {
      assertInBounds();
      return Iterator(queue_, nextIndex(index_));
    }

   private:
    void assertInBounds() const {
      if (queue_) {
        if (queue_->firstIndex_ < queue_->lastIndex_) {
          assert((index_ >= queue_->firstIndex_) &&
                 (index_ < queue_->lastIndex_));
        } else {
          assert(!((index_ < queue_->firstIndex_) &&
                   (index_ >= queue_->lastIndex_)));
        }
      }
    }

   private:
    const PooledQueue *queue_; /*!< Store a reference to the queue. nullptr
                                  represents an invalid iterator. */
    uint16_t index_;           /*!< Store the location within the queue. */
  };

 public:
  PooledQueue() { static_assert(N > 1); }

  inline Iterator first() const {
    return Iterator(firstIndex_ == lastIndex_ ? nullptr : this, firstIndex_);
  }
  inline Iterator last() const {
    return Iterator(firstIndex_ == lastIndex_ ? nullptr : this,
                    prevIndex(lastIndex_));
  }
  inline uint16_t size() const {
    int16_t diff = (int16_t)lastIndex_ - (int16_t)firstIndex_;
    return diff < 0 ? diff + N : diff;
  }
  inline uint16_t getNumSpacesLeft() const { return N - size() - 1; }

  /**
   * Allocate a spot in the queue and return the iterator. The iterator is
   * invalid if there is no capacity left.
   */
  inline Iterator push() {
    uint16_t nextLastIndex = nextIndex(lastIndex_);
    if (nextLastIndex == firstIndex_) {
      return Iterator(nullptr, 0);
    }
    uint16_t oldLastIndex = lastIndex_;
    lastIndex_ = nextLastIndex;
    return Iterator(this, oldLastIndex);
  }

  /**
   * Allocate a spot in the queue and set the value. Returns whether the
   * operation was successful.
   */
  inline bool push(const T &data) {
    Iterator it = push();
    if (it) {
      *it = data;
    }
    return static_cast<bool>(it);
  }

  /**
   * Free the first element in the queue. Returns whether the operation was
   * successful. The memory of the old item is not reset.
   */
  inline bool pop() {
    bool canPop = firstIndex_ != lastIndex_;
    if (canPop) {
      firstIndex_ = nextIndex(firstIndex_);
    }
    return canPop;
  }

 private:
  inline static uint16_t nextIndex(const uint16_t index) {
    return (index + 1) % N;
  }
  inline static uint16_t prevIndex(const uint16_t index) {
    return (index + N - 1) % N;
  }

 private:
  T data_[N]; /*!< Hold the data in a statically-allocated array. */
  uint16_t firstIndex_ = 0; /*!< Index of the first item in the queue; indices
                           are in the range [0, N) */
  uint16_t lastIndex_ = 0; /*!< Index + 1 of the last item in the queue; indices
                          are in the range [0, N) */
};
}  // namespace Clef::Util
