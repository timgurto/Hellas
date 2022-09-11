#pragma once

#include <string>

struct Options {
  Options() { getFilePath(); }
  void save();
  void load();

  struct Graphics {
    bool fullScreen{false};
  } graphics;

 private:
  void getFilePath();
  std::string m_filePath;
};
