// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <util/PooledQueue.h>

namespace {
struct Dummy {
  uint16_t x;
  uint16_t y;
  bool operator==(const Dummy &other) const {
    return x == other.x && y == other.y;
  }
};
}  // namespace

namespace Clef::Util {
TEST(PooledQueueTest, ZeroSize) {
  PooledQueue<Dummy, 8> queue;
  ASSERT_EQ(queue.size(), 0);
  ASSERT_FALSE(queue.first());
  ASSERT_FALSE(queue.last());
  ASSERT_FALSE(queue.pop());
}

TEST(PooledQueueTest, Push) {
  using Queue = PooledQueue<Dummy, 4>;
  Queue queue;

  Queue::Iterator it0 = queue.push();
  ASSERT_TRUE(it0);
  it0->x = 4;
  it0->y = 8;
  ASSERT_EQ(queue.size(), 1);
  ASSERT_EQ(it0, queue.first());
  ASSERT_EQ(*it0, *queue.first());
  ASSERT_EQ(it0, queue.last());
  ASSERT_EQ(*it0, *queue.last());

  ASSERT_TRUE(queue.push({17, 37}));
  ASSERT_EQ(queue.size(), 2);
  Queue::Iterator it1 = queue.first().next();
  ASSERT_EQ(it1, queue.last());
  ASSERT_EQ(*it1, *queue.last());

  ASSERT_TRUE(queue.push({42, 56}));
  ASSERT_EQ(queue.size(), 3);
  Queue::Iterator it2 = queue.first().next().next();
  ASSERT_EQ(*it2, *queue.last());
  ASSERT_FALSE(queue.push({0, 0}));

  ASSERT_EQ(it0->x, 4);
  ASSERT_EQ(it0->y, 8);
  ASSERT_EQ(it1->x, 17);
  ASSERT_EQ(it1->y, 37);
  ASSERT_EQ(it2->x, 42);
  ASSERT_EQ(it2->y, 56);
}

TEST(PooledQueueTest, Pop) {
  using Queue = PooledQueue<Dummy, 4>;
  Queue queue;
  ASSERT_TRUE(queue.push({0, 0}));
  ASSERT_EQ(queue.size(), 1);
  ASSERT_TRUE(queue.pop());
  ASSERT_FALSE(queue.pop());
  ASSERT_EQ(queue.size(), 0);

  ASSERT_TRUE(queue.push({1, 1}));
  ASSERT_TRUE(queue.push({2, 2}));
  ASSERT_EQ(queue.size(), 2);
  ASSERT_TRUE(queue.pop());
  ASSERT_EQ(queue.size(), 1);
  ASSERT_TRUE(queue.pop());
  ASSERT_FALSE(queue.pop());
  ASSERT_EQ(queue.size(), 0);

  ASSERT_TRUE(queue.push({3, 3}));
  ASSERT_TRUE(queue.push({4, 4}));
  ASSERT_TRUE(queue.push({5, 5}));
  ASSERT_EQ(queue.size(), 3);
  ASSERT_TRUE(queue.pop());
  ASSERT_EQ(queue.size(), 2);
  ASSERT_TRUE(queue.pop());
  ASSERT_EQ(queue.size(), 1);
  ASSERT_TRUE(queue.pop());
  ASSERT_FALSE(queue.pop());
  ASSERT_EQ(queue.size(), 0);
}
}  // namespace Clef::Util
