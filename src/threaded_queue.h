#pragma once

#include <optional.h>

#include <algorithm>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

// TODO: cleanup includes.

struct BaseThreadQueue {
  virtual bool IsEmpty() = 0;
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
  ThreadedQueue() {
    owned_waiter_ = MakeUnique<MultiQueueWaiter>();
    waiter_ = owned_waiter_.get();
  }

  explicit ThreadedQueue(MultiQueueWaiter* waiter) : waiter_(waiter) {}

  // Add an element to the front of the queue.
  void PriorityEnqueue(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    priority_.push(std::move(t));
    waiter_->cv.notify_all();
  }

  // Add an element to the queue.
  void Enqueue(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(t));
    waiter_->cv.notify_all();
  }

  // Return all elements in the queue.
  std::vector<T> DequeueAll() {
    std::lock_guard<std::mutex> lock(mutex_);

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

  bool IsEmpty() {
    std::lock_guard<std::mutex> lock(mutex_);
    return priority_.empty() && queue_.empty();
  }

  // Get the first element from the queue. Blocks until one is available.
  // Executes |action| with an acquired |mutex_|.
  template<typename TAction>
  T DequeuePlusAction(TAction& action) {
    std::unique_lock<std::mutex> lock(mutex_);
    waiter_->cv.wait(lock, [&]() {
      return !priority_.empty() || !queue_.empty();
    });

    if (!priority_.empty()) {
      auto val = std::move(priority_.front());
      priority_.pop();
      return std::move(val);
    }

    auto val = std::move(queue_.front());
    queue_.pop();

    action();

    return std::move(val);
  }

  // Get the first element from the queue. Blocks until one is available.
  T Dequeue() {
    return DequeuePlusAction([]() {});
  }

  // Get the first element from the queue without blocking. Returns a null
  // value if the queue is empty.
  optional<T> TryDequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (priority_.empty() && queue_.empty())
      return nullopt;

    if (!priority_.empty()) {
      auto val = std::move(priority_.front());
      priority_.pop();
      return std::move(val);
    }

    auto val = std::move(queue_.front());
    queue_.pop();
    return std::move(val);
  }

 private:
  std::queue<T> priority_;
  mutable std::mutex mutex_;
  std::queue<T> queue_;
  MultiQueueWaiter* waiter_;
  std::unique_ptr<MultiQueueWaiter> owned_waiter_;
};
