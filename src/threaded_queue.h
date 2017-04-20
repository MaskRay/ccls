#pragma once

#include <optional.h>

#include <algorithm>
#include <queue>
#include <mutex>
#include <condition_variable>

// TODO: cleanup includes.


// A threadsafe-queue. http://stackoverflow.com/a/16075550
template <class T>
class ThreadedQueue {
public:
  // Add an element to the front of the queue.
  void PriorityEnqueue(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    priority_.push(std::move(t));
    cv_.notify_one();
  }

  // Add an element to the queue.
  void Enqueue(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(t));
    cv_.notify_one();
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

  // Get the "front"-element.
  // If the queue is empty, wait untill an element is avaiable.
  T Dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (priority_.empty() && queue_.empty()) {
      // release lock as long as the wait and reaquire it afterwards.
      cv_.wait(lock);
    }
    
    if (!priority_.empty()) {
      auto val = std::move(priority_.front());
      priority_.pop();
      return val;
    }

    auto val = std::move(queue_.front());
    queue_.pop();
    return val;
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
      return val;
    }

    auto val = std::move(queue_.front());
    queue_.pop();
    return std::move(val);
  }

 private:
  std::queue<T> priority_;
  std::queue<T> queue_;
  mutable std::mutex mutex_;
  std::condition_variable cv_;
};
