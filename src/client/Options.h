#pragma once

#include <string>

struct Options {
  Options() { getFilePath(); }
  void save();
  void load();

 private:
  void getFilePath();
  std::string m_filePath;
};
