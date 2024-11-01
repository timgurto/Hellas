#pragma once

#include <string>
#include <thread>

void setThreadName(uint32_t dwThreadID, const char *threadName);
void setThreadName(const char *threadName);
void setThreadName(std::thread *thread, const char *threadName);

void setThreadName(const std::string &threadName);
