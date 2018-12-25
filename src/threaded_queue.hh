/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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
  virtual bool IsEmpty() = 0;
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

  static bool HasState(std::initializer_list<BaseThreadQueue *> queues) {
    for (BaseThreadQueue *queue : queues) {
      if (!queue->IsEmpty())
        return true;
    }
    return false;
  }

  template <typename... BaseThreadQueue>
  bool Wait(std::atomic<bool> &quit, BaseThreadQueue... queues) {
    MultiQueueLock<BaseThreadQueue...> l(queues...);
    while (!quit.load(std::memory_order_relaxed)) {
      if (HasState({queues...}))
        return false;
      cv.wait(l);
    }
    return true;
  }

  template <typename... BaseThreadQueue>
  void WaitUntil(std::chrono::steady_clock::time_point t,
                 BaseThreadQueue... queues) {
    MultiQueueLock<BaseThreadQueue...> l(queues...);
    if (!HasState({queues...}))
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
  size_t Size() const { return total_count_; }

  // Add an element to the queue.
  template <void (std::deque<T>::*push)(T &&)> void Push(T &&t, bool priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (priority)
      (priority_.*push)(std::move(t));
    else
      (queue_.*push)(std::move(t));
    ++total_count_;
    waiter_->cv.notify_one();
  }

  void PushBack(T &&t, bool priority = false) {
    Push<&std::deque<T>::push_back>(std::move(t), priority);
  }

  // Return all elements in the queue.
  std::vector<T> DequeueAll() {
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
  bool IsEmpty() { return total_count_ == 0; }

  // Get the first element from the queue. Blocks until one is available.
  T Dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    waiter_->cv.wait(lock,
                     [&]() { return !priority_.empty() || !queue_.empty(); });

    auto execute = [&](std::deque<T> *q) {
      auto val = std::move(q->front());
      q->pop_front();
      --total_count_;
      return std::move(val);
    };
    if (!priority_.empty())
      return execute(&priority_);
    return execute(&queue_);
  }

  // Get the first element from the queue without blocking. Returns a null
  // value if the queue is empty.
  std::optional<T> TryPopFront() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto execute = [&](std::deque<T> *q) {
      auto val = std::move(q->front());
      q->pop_front();
      --total_count_;
      return std::move(val);
    };
    if (priority_.size())
      return execute(&priority_);
    if (queue_.size())
      return execute(&queue_);
    return std::nullopt;
  }

  template <typename Fn> void Iterate(Fn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &entry : priority_)
      fn(entry);
    for (auto &entry : queue_)
      fn(entry);
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
