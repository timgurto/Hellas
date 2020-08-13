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

WorkerThread& WorkerThread::enqueue(WorkerThread::Task newTask) {
  _tasks.push(newTask);
  return *this;
}

WorkerThread& WorkerThread::waitUntilDone() {
  while (!_tasks.empty())
    ;
  return *this;
}

void WorkerThread::assertIsExecutingThis() {
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
