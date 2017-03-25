#pragma once

// TODO: cleanup includes.
#include <algorithm>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "../optional.h"

// A threadsafe-queue. http://stackoverflow.com/a/16075550
template <class T>
class ThreadedQueue {
public:
  // Add an element to the queue.
  void Enqueue(T t) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(t);
    cv_.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  T Dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      // release lock as long as the wait and reaquire it afterwards.
      cv_.wait(lock);
    }
    T val = queue_.front();
    queue_.pop();
    return val;
  }

  // Get the "front"-element.
  // Returns empty if the queue is empty.
  optional<T> TryDequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty())
      return nullopt;

    T val = queue_.front();
    queue_.pop();
    return val;
  }

private:
  std::queue<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};
