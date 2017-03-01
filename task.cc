#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>

#include "indexer.h"
#include "query.h"
#include "optional.h"
#include "third_party/tiny-process-library/process.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>

using std::experimental::optional;
using std::experimental::nullopt;

// A threadsafe-queue. http://stackoverflow.com/a/16075550
template <class T>
class SafeQueue {
public:
  // Add an element to the queue.
  void enqueue(T t) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(t);
    cv_.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  T dequeue() {
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
  optional<T> try_dequeue() {
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

struct Task {
  int priority = 0;
  bool writes_to_index = false;
  bool should_exit = false;

  static Task MakeExit() {
    Task task;
    task.should_exit = true;
    return task;
  }

  // TODO: Create index task.
  // Task running in a separate process, parsing a file into something we can
  // import.

  // TODO: Index import task.
  // Completed parse task that wants to import content into the global database.
  // Runs in main process, primary thread. Stops all other threads.

  // TODO: Index fresh task.
  // Completed parse task that wants to update content previously imported into
  // the global database. Runs in main process, primary thread. Stops all other
  // threads.
  //
  // Note that this task just contains a set of operations to apply to the global
  // database. The operations come from a diff based on the previously indexed
  // state in comparison to the newly indexed state.
  //
  // TODO: We may be able to run multiple freshen and import tasks in parallel if
  //       we restrict what ranges of the db they may change.

  // TODO: QueryTask
  // Task running a query against the global database. Run in main process,
  // separate thread.
  Command query;
  Location location;
  std::string argument;
};

// NOTE: When something enters a value into master db, it will have to have a
//       ref count, since multiple parsings could enter it (unless we require
//       that it be defined in that declaration unit!)
struct TaskManager {
  SafeQueue<Task> queued_tasks;

  // Available threads.
  std::vector<std::thread> threads;

  TaskManager(int num_threads);
};

static void ThreadMain(int id, TaskManager* tm) {
  while (true) {
    Task task = tm->queued_tasks.dequeue();
    if (task.should_exit) {
      std::cout << id << ": Exiting" << std::endl;
      return;
    }

    std::cout << id << ": waking" << std::endl;
  }

}

TaskManager::TaskManager(int num_threads) {
  for (int i = 0; i < num_threads; ++i) {
    threads.push_back(std::thread(&ThreadMain, i, this));
  }
}

void Pump(TaskManager* tm) {
  //tm->threads[0].
}

int main5555555555(int argc, char** argv) {
  TaskManager tm(5);

  // TODO: looks like we will have to write shared memory support.

  // TODO: We signal thread to pick data, thread signals data pick is done.
  //       Repeat until we encounter a writer, wait for all threads to signal
  //       they are done.
  // TODO: Let's use a thread safe queue/vector/etc instead.
  for (int i = 0; i < 10; ++i)
    tm.queued_tasks.enqueue(Task::MakeExit());

  for (std::thread& thread : tm.threads)
    thread.join();

  std::cin.get();
  return 0;
}