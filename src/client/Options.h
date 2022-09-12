#pragma once

#include <string>

class Client;

struct Options {
  Options() { getFilePath(); }
  void save();
  void load();
  void onAnyChange(Client& client) const;

  struct Video {
    bool fullScreen{false};
  } video;

  struct Audio {
    bool enableSFX{true};
  } audio;

  struct UI {
    bool uniformHealthBars{false};
    bool showQuestProgress{true};
  } ui;

  struct Parental {
    bool showCustomNames{true};
  } parental;

 private:
  void getFilePath();
  std::string m_filePath;
};
