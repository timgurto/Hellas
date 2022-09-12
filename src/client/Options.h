#pragma once

#include <string>

struct Options {
  Options() { getFilePath(); }
  void save();
  void load();

  struct Video {
    bool fullScreen{false};
  } video;

  struct Audio {
    bool enableSFX{true};
  } audio;

  struct UI {
    bool uniformHealthBars{false};
  } ui;

  struct Parental {
    bool showCustomNames{true};
  } parental;

 private:
  void getFilePath();
  std::string m_filePath;
};
