#pragma once

#include "utils.h"
#include "work_thread.h"

#include <optional.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <tuple>

// TODO: cleanup includes.

struct BaseThreadQueue {
  virtual bool IsEmpty() = 0;
  virtual ~BaseThreadQueue() = default;
};

// TODO Remove after migration to C++14
namespace {

template <size_t... Is>
struct index_sequence {};

template <size_t I, size_t... Is>
struct make_index_sequence {
  using type = typename make_index_sequence<I-1, I-1, Is...>::type;
};

template <size_t... Is>
struct make_index_sequence<0, Is...> {
  using type = index_sequence<Is...>;
};

}

// std::lock accepts two or more arguments. We define an overload for one
// argument.
namespace std {
template <typename Lockable>
void lock(Lockable& l) {
  l.lock();
}
}

template <typename... Queue>
struct MultiQueueLock {
  MultiQueueLock(Queue... lockable) : tuple_{lockable...} {
    lock();
  }
  ~MultiQueueLock() {
    unlock();
  }
  void lock() {
    lock_impl(typename make_index_sequence<sizeof...(Queue)>::type{});
  }
  void unlock() {
    unlock_impl(typename make_index_sequence<sizeof...(Queue)>::type{});
  }

 private:
  template <size_t... Is>
  void lock_impl(index_sequence<Is...>) {
    std::lock(std::get<Is>(tuple_)->mutex_...);
  }

  template <size_t... Is>
  void unlock_impl(index_sequence<Is...>) {
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

  mutable std::mutex mutex_;

 private:
  std::atomic<int> total_count_;
  std::queue<T> priority_;
  std::queue<T> queue_;
  MultiQueueWaiter* waiter_;
  std::unique_ptr<MultiQueueWaiter> owned_waiter_;
};
