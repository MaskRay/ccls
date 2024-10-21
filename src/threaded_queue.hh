// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils.hh"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>

// std::lock accepts two or more arguments. We define an overload for one
// argument.
namespace std {
template <typename Lockable> void lock(Lockable &l) { l.lock(); }
} // namespace std

namespace ccls {
struct BaseThreadQueue {
  virtual bool isEmpty() = 0;
  virtual ~BaseThreadQueue() = default;
};

template <typename... Queue> struct MultiQueueLock {
  MultiQueueLock(Queue... lockable) : tuple_{lockable...} { lock(); }
  ~MultiQueueLock() { unlock(); }
  void lock() { lock_impl(typename std::index_sequence_for<Queue...>{}); }
  void unlock() { unlock_impl(typename std::index_sequence_for<Queue...>{}); }

private:
  template <size_t... Is> void lock_impl(std::index_sequence<Is...>) {
    std::lock(std::get<Is>(tuple_)->mutex_...);
  }

  template <size_t... Is> void unlock_impl(std::index_sequence<Is...>) {
    (std::get<Is>(tuple_)->mutex_.unlock(), ...);
  }

  std::tuple<Queue...> tuple_;
};

struct MultiQueueWaiter {
  std::condition_variable_any cv;

  static bool hasState(std::initializer_list<BaseThreadQueue *> queues) {
    for (BaseThreadQueue *queue : queues) {
      if (!queue->isEmpty())
        return true;
    }
    return false;
  }

  template <typename... BaseThreadQueue>
  bool wait(std::atomic<bool> &quit, BaseThreadQueue... queues) {
    MultiQueueLock<BaseThreadQueue...> l(queues...);
    while (!quit.load(std::memory_order_relaxed)) {
      if (hasState({queues...}))
        return false;
      cv.wait(l);
    }
    return true;
  }

  template <typename... BaseThreadQueue>
  void waitUntil(std::chrono::steady_clock::time_point t,
                 BaseThreadQueue... queues) {
    MultiQueueLock<BaseThreadQueue...> l(queues...);
    if (!hasState({queues...}))
      cv.wait_until(l, t);
  }
};

// A threadsafe-queue. http://stackoverflow.com/a/16075550
template <class T> struct ThreadedQueue : public BaseThreadQueue {
public:
  ThreadedQueue() {
    owned_waiter_ = std::make_unique<MultiQueueWaiter>();
    waiter_ = owned_waiter_.get();
  }
  explicit ThreadedQueue(MultiQueueWaiter *waiter) : waiter_(waiter) {}

  // Returns the number of elements in the queue. This is lock-free.
  size_t size() const { return total_count_; }

  // Add an element to the queue.
  template <void (std::deque<T>::*Push)(T &&)> void push(T &&t, bool priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (priority)
      (priority_.*Push)(std::move(t));
    else
      (queue_.*Push)(std::move(t));
    ++total_count_;
    waiter_->cv.notify_one();
  }

  void pushBack(T &&t, bool priority = false) {
    push<&std::deque<T>::push_back>(std::move(t), priority);
  }

  // Return all elements in the queue.
  std::vector<T> dequeueAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    total_count_ = 0;

    std::vector<T> result;
    result.reserve(priority_.size() + queue_.size());
    while (!priority_.empty()) {
      result.emplace_back(std::move(priority_.front()));
      priority_.pop_front();
    }
    while (!queue_.empty()) {
      result.emplace_back(std::move(queue_.front()));
      queue_.pop_front();
    }

    return result;
  }

  // Returns true if the queue is empty. This is lock-free.
  bool isEmpty() { return total_count_ == 0; }

  // Get the first element from the queue. Blocks until one is available.
  T dequeue(int keep_only_latest = 0) {
    std::unique_lock<std::mutex> lock(mutex_);
    waiter_->cv.wait(lock,
                     [&]() { return !priority_.empty() || !queue_.empty(); });

    auto execute = [&](std::deque<T> *q) {
      if (keep_only_latest > 0 && q->size() > keep_only_latest)
        q->erase(q->begin(), q->end() - keep_only_latest);
      auto val = std::move(q->front());
      q->pop_front();
      --total_count_;
      return val;
    };
    if (!priority_.empty())
      return execute(&priority_);
    return execute(&queue_);
  }

  // Get the first element from the queue without blocking. Returns a null
  // value if the queue is empty.
  std::optional<T> tryPopFront() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto execute = [&](std::deque<T> *q) {
      auto val = std::move(q->front());
      q->pop_front();
      --total_count_;
      return val;
    };
    if (priority_.size())
      return execute(&priority_);
    if (queue_.size())
      return execute(&queue_);
    return std::nullopt;
  }

  template <typename Fn> void apply(Fn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    fn(queue_);
  }

  mutable std::mutex mutex_;

private:
  std::atomic<int> total_count_{0};
  std::deque<T> priority_;
  std::deque<T> queue_;
  MultiQueueWaiter *waiter_;
  std::unique_ptr<MultiQueueWaiter> owned_waiter_;
};
} // namespace ccls
