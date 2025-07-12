#include "Log.h"

static void createEmptyFile(const std::string &filename) {
  std::ofstream of(filename);
  of.close();
}

bool Log::usingLogFile() const { return _logFile; }

Log::Log(const std::string &logFileName) {
  if (!logFileName.empty()) createEmptyFile(logFileName);
}

FileAppender::FileAppender(const std::string &logFileName) {
  if (logFileName.empty()) return;
  _fileStream.open(logFileName, std::ios_base::app);
}

FileAppender::~FileAppender() { _fileStream.close(); }
