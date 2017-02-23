#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>

#include "indexer.h"
#include "query.h"

#include "third_party/tiny-process-library/process.hpp"

struct BaseTask {
  int priority = 0;
  bool writes_to_index = false;
};

// Task running in a separate process, parsing a file into something we can
// import.
struct IndexCreateTask : public BaseTask {
  IndexCreateTask() {
    writes_to_index = true;
  }
};

// Completed parse task that wants to import content into the global database.
// Runs in main process, primary thread. Stops all other threads.
struct IndexImportTask : public BaseTask {
  IndexImportTask() {
    writes_to_index = true;
  }
};

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
struct IndexFreshenTask : public BaseTask {
  IndexFreshenTask() {
    writes_to_index = true;
  }
};

// Task running a query against the global database. Run in main process,
// separate thread.
struct QueryTask : public BaseTask {
  QueryTask() {
    writes_to_index = false;
  }

  Command query;
  Location location;
  std::string argument;
};


// NOTE: When something enters a value into master db, it will have to have a
//       ref count, since multiple parsings could enter it (unless we require
//       that it be defined in that declaration unit!)
struct TaskManager {
  // Tasks that are currently executing.
  std::vector<BaseTask> running;
  std::vector<BaseTask> pending;

  // Available threads.
  std::vector<std::thread> threads;
  std::condition_variable wakeup_thread;
  std::mutex mutex;

  TaskManager(int num_threads);
};

static void ThreadMain(int id, std::condition_variable* waiter, std::mutex* mutex) {
  std::unique_lock<std::mutex> lock(*mutex);
  waiter->wait(lock);

  std::cout << id << ": running in thread main" << std::endl;
}

TaskManager::TaskManager(int num_threads) {
  for (int i = 0; i < num_threads; ++i) {
    threads.push_back(std::thread(&ThreadMain, i, &wakeup_thread, &mutex));
  }
}

void Pump(TaskManager* tm) {
  //tm->threads[0].
}

int main(int argc, char** argv) {
  TaskManager tm(10);

  // TODO: looks like we will have to write shared memory support.

  // TODO: We signal thread to pick data, thread signals data pick is done.
  //       Repeat until we encounter a writer, wait for all threads to signal
  //       they are done.
  // TODO: Let's use a thread safe queue/vector/etc instead.
  tm.wakeup_thread.notify_one();
  tm.wakeup_thread.notify_one();

  for (std::thread& thread : tm.threads)
    thread.join();

  std::cin.get();
  return 0;
}