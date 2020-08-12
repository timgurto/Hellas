#include "WorkerThread.h"

#include <SDL.h>

#include <cassert>

#include "../threadNaming.h"

WorkerThread::WorkerThread(const std::string& threadName) {
  std::thread([&]() {
    setThreadName(threadName);
    _workerThreadID = std::this_thread::get_id();
    run();
  }).detach();
}

void WorkerThread::enqueue(WorkerThread::Task newTask) { _tasks.push(newTask); }

void WorkerThread::waitUntilDone() {
  while (!_tasks.empty()) {
  }
}

void WorkerThread::requireThisCallToBeInWorkerThread() {
  auto thisThreadID = std::this_thread::get_id();
  assert(thisThreadID == _workerThreadID);
}

void WorkerThread::run() {
  while (true) {
    if (_tasks.empty()) {
      SDL_Delay(1);
      continue;
    }

    auto nextTask = _tasks.front();
    nextTask();
    _tasks.pop();
  }
}
