#pragma once

#include "utils.h"
#include "work_thread.h"

#include <optional.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

// TODO: cleanup includes.

struct BaseThreadQueue {
  virtual bool IsEmpty() = 0;
  virtual ~BaseThreadQueue() = default;
};

struct MultiQueueWaiter {
  std::mutex m;
  std::condition_variable cv;

  bool HasState(std::initializer_list<BaseThreadQueue*> queues) {
    for (BaseThreadQueue* queue : queues) {
      if (!queue->IsEmpty())
        return true;
    }
    return false;
  }

  void Wait(std::initializer_list<BaseThreadQueue*> queues) {
    // We cannot have a single condition variable wait on all of the different
    // mutexes, so we have a global condition variable that every queue will
    // notify. After it is notified we check to see if any of the queues have
    // data; if they do, we return.
    //
    // We repoll every 5 seconds because it's not possible to atomically check
    // the state of every queue and then setup the condition variable. So, if
    // Wait() is called, HasState() returns false, and then in the time after
    // HasState() is called data gets posted but before we begin waiting for
    // the condition variable, we will miss the notification. The timeout of 5
    // means that if this happens we will delay operation for 5 seconds.

    while (!HasState(queues)) {
      std::unique_lock<std::mutex> l(m);
      cv.wait_for(l, std::chrono::seconds(5));
    }
  }
};

// A threadsafe-queue. http://stackoverflow.com/a/16075550
template <class T>
struct ThreadedQueue : public BaseThreadQueue {
 public:
  ThreadedQueue() : total_count_(0) {
    owned_waiter_ = MakeUnique<MultiQueueWaiter>();
    waiter_ = owned_waiter_.get();
  }

  explicit ThreadedQueue(MultiQueueWaiter* waiter)
      : total_count_(0), waiter_(waiter) {}

  // Returns the number of elements in the queue. This is lock-free.
  size_t Size() const { return total_count_; }

  // Add an element to the front of the queue.
  void PriorityEnqueue(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    priority_.push(std::move(t));
    ++total_count_;
    waiter_->cv.notify_all();
  }

  // Add an element to the queue.
  void Enqueue(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(t));
    ++total_count_;
    waiter_->cv.notify_all();
  }

  // Add a set of elements to the queue.
  void EnqueueAll(std::vector<T>&& elements) {
    std::lock_guard<std::mutex> lock(mutex_);

    total_count_ += elements.size();

    for (T& element : elements) {
      queue_.push(std::move(element));
    }
    elements.clear();

    waiter_->cv.notify_all();
  }

  // Return all elements in the queue.
  std::vector<T> DequeueAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    total_count_ = 0;

    std::vector<T> result;
    result.reserve(priority_.size() + queue_.size());
    while (!priority_.empty()) {
      result.emplace_back(std::move(priority_.front()));
      priority_.pop();
    }
    while (!queue_.empty()) {
      result.emplace_back(std::move(queue_.front()));
      queue_.pop();
    }

    return result;
  }

  // Returns true if the queue is empty. This is lock-free.
  bool IsEmpty() { return total_count_ == 0; }

  // TODO: Unify code between DequeuePlusAction with TryDequeuePlusAction.
  // Probably have opt<T> Dequeue(bool wait_for_element);

  // Get the first element from the queue. Blocks until one is available.
  // Executes |action| with an acquired |mutex_|.
  template <typename TAction>
  T DequeuePlusAction(TAction action) {
    std::unique_lock<std::mutex> lock(mutex_);
    waiter_->cv.wait(lock,
                     [&]() { return !priority_.empty() || !queue_.empty(); });

    auto execute = [&](std::queue<T>* q) {
      auto val = std::move(q->front());
      q->pop();
      --total_count_;

      action();

      return std::move(val);
    };
    if (!priority_.empty())
      return execute(&priority_);
    return execute(&queue_);
  }

  // Get the first element from the queue. Blocks until one is available.
  T Dequeue() {
    return DequeuePlusAction([]() {});
  }

  // Get the first element from the queue without blocking. Returns a null
  // value if the queue is empty.
  template <typename TAction>
  optional<T> TryDequeuePlusAction(TAction action) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (priority_.empty() && queue_.empty())
      return nullopt;

    auto execute = [&](std::queue<T>* q) {
      auto val = std::move(q->front());
      q->pop();
      --total_count_;

      action(val);

      return std::move(val);
    };
    if (!priority_.empty())
      return execute(&priority_);
    return execute(&queue_);
  }

  optional<T> TryDequeue() {
    return TryDequeuePlusAction([](const T&) {});
  }

 private:
  std::atomic<int> total_count_;
  std::queue<T> priority_;
  mutable std::mutex mutex_;
  std::queue<T> queue_;
  MultiQueueWaiter* waiter_;
  std::unique_ptr<MultiQueueWaiter> owned_waiter_;
};
