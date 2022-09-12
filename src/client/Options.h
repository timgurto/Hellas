#pragma once

#include <string>

struct Options {
  Options() { getFilePath(); }
  void save();
  void load();

  struct Graphics {
    bool fullScreen{false};
  } graphics;

  struct Parental {
    bool showCustomNames{true};
  } parental;

 private:
  void getFilePath();
  std::string m_filePath;
};
