#pragma once

#include "utils.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>

struct BaseThreadQueue {
  virtual bool IsEmpty() = 0;
  virtual ~BaseThreadQueue() = default;
};

// std::lock accepts two or more arguments. We define an overload for one
// argument.
namespace std {
template <typename Lockable>
void lock(Lockable& l) {
  l.lock();
}
}  // namespace std

template <typename... Queue>
struct MultiQueueLock {
  MultiQueueLock(Queue... lockable) : tuple_{lockable...} { lock(); }
  ~MultiQueueLock() { unlock(); }
  void lock() { lock_impl(typename std::index_sequence_for<Queue...>{}); }
  void unlock() { unlock_impl(typename std::index_sequence_for<Queue...>{}); }

 private:
  template <size_t... Is>
  void lock_impl(std::index_sequence<Is...>) {
    std::lock(std::get<Is>(tuple_)->mutex_...);
  }

  template <size_t... Is>
  void unlock_impl(std::index_sequence<Is...>) {
    (void)std::initializer_list<int>{
        (std::get<Is>(tuple_)->mutex_.unlock(), 0)...};
  }

  std::tuple<Queue...> tuple_;
};

struct MultiQueueWaiter {
  std::condition_variable_any cv;

  static bool HasState(std::initializer_list<BaseThreadQueue*> queues) {
    for (BaseThreadQueue* queue : queues) {
      if (!queue->IsEmpty())
        return true;
    }
    return false;
  }

  template <typename... BaseThreadQueue>
  void Wait(BaseThreadQueue... queues) {
    MultiQueueLock<BaseThreadQueue...> l(queues...);
    while (!HasState({queues...}))
      cv.wait(l);
  }
};

// A threadsafe-queue. http://stackoverflow.com/a/16075550
template <class T>
struct ThreadedQueue : public BaseThreadQueue {
 public:
  ThreadedQueue() : total_count_(0) {
    owned_waiter_ = std::make_unique<MultiQueueWaiter>();
    waiter_ = owned_waiter_.get();
    owned_waiter1_ = std::make_unique<MultiQueueWaiter>();
    waiter1_ = owned_waiter1_.get();
  }

  // TODO remove waiter1 after split of on_indexed
  explicit ThreadedQueue(MultiQueueWaiter* waiter,
                         MultiQueueWaiter* waiter1 = nullptr)
      : total_count_(0), waiter_(waiter), waiter1_(waiter1) {}

  // Returns the number of elements in the queue. This is lock-free.
  size_t Size() const { return total_count_; }

  // Add an element to the queue.
  template <void (std::deque<T>::*push)(T&&)>
  void Push(T&& t, bool priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (priority)
      (priority_.*push)(std::move(t));
    else
      (queue_.*push)(std::move(t));
    ++total_count_;
    waiter_->cv.notify_one();
    if (waiter1_)
      waiter1_->cv.notify_one();
  }

  void PushFront(T&& t, bool priority = false) {
    Push<&std::deque<T>::push_front>(std::move(t), priority);
  }

  void PushBack(T&& t, bool priority = false) {
    Push<&std::deque<T>::push_back>(std::move(t), priority);
  }

  // Add a set of elements to the queue.
  void EnqueueAll(std::vector<T>&& elements, bool priority = false) {
    if (elements.empty())
      return;

    std::lock_guard<std::mutex> lock(mutex_);

    total_count_ += elements.size();

    for (T& element : elements) {
      if (priority)
        priority_.push_back(std::move(element));
      else
        queue_.push_back(std::move(element));
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

    auto execute = [&](std::deque<T>* q) {
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
  std::optional<T> TryPopFrontHelper(int which) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto execute = [&](std::deque<T>* q) {
      auto val = std::move(q->front());
      q->pop_front();
      --total_count_;
      return std::move(val);
    };
    if (which & 2 && priority_.size())
      return execute(&priority_);
    if (which & 1 && queue_.size())
      return execute(&queue_);
    return std::nullopt;
  }

  std::optional<T> TryPopFront() { return TryPopFrontHelper(3); }

  std::optional<T> TryPopBack() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto execute = [&](std::deque<T>* q) {
      auto val = std::move(q->back());
      q->pop_back();
      --total_count_;
      return std::move(val);
    };
    // Reversed
    if (queue_.size())
      return execute(&queue_);
    if (priority_.size())
      return execute(&priority_);
    return std::nullopt;
  }

  std::optional<T> TryPopFrontLow() { return TryPopFrontHelper(1); }

  std::optional<T> TryPopFrontHigh() { return TryPopFrontHelper(2); }

  template <typename Fn>
  void Iterate(Fn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& entry : priority_)
      fn(entry);
    for (auto& entry : queue_)
      fn(entry);
  }

  mutable std::mutex mutex_;

 private:
  std::atomic<int> total_count_;
  std::deque<T> priority_;
  std::deque<T> queue_;
  MultiQueueWaiter* waiter_;
  std::unique_ptr<MultiQueueWaiter> owned_waiter_;
  // TODO remove waiter1 after split of on_indexed
  MultiQueueWaiter* waiter1_;
  std::unique_ptr<MultiQueueWaiter> owned_waiter1_;
};
