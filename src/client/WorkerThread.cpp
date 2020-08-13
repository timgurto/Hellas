#include "WorkerThread.h"

#include <SDL.h>

#include <cassert>

#include "../threadNaming.h"

WorkerThread::WorkerThread(const std::string& threadName) {
  std::thread([&]() {
    setThreadName(threadName);
    run();
  }).detach();
}

void WorkerThread::callBlocking(Task task) {
  assert(!_pendingTask);
  _pendingTask = task;

  while (_pendingTask)
    ;
}

void WorkerThread::run() {
  while (true) {
    if (_pendingTask) {
      _pendingTask();
      _pendingTask = nullptr;
    }

    // SDL_Delay(1);
  }
}
