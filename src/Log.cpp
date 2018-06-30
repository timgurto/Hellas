#include "Log.h"

Log::Log(const std::string &logFileName) : _logFileName(logFileName) {
  if (!logFileName.empty()) {
    std::ofstream of(logFileName);
    of.close();
  }
}

bool Log::usingLogFile() const { return _logFileName.empty(); }

FileAppender Log::logFile() const { return {_logFileName}; }

FileAppender::FileAppender(const std::string &logFileName) {
  if (logFileName.empty()) return;
  _fileStream.open(logFileName, std::ios_base::app);
}

FileAppender::~FileAppender() { _fileStream.close(); }
