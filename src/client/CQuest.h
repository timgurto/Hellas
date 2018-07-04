#pragma once

#include <map>
#include <string>

class Window;

class CQuest {
 public:
  using ID = std::string;
  using Name = std::string;

  CQuest(const ID &id = "") : _id(id) { generateWindow(); }

  bool operator<(const CQuest &rhs) { return _id < rhs._id; }

  const ID &id() const { return _id; }
  const Name &name() const { return _name; }
  void name(const Name &newName) { _name = newName; }
  const Window *window() const { return _window; }

  void generateWindow();

 private:
  ID _id;
  Name _name;
  Window *_window{nullptr};
};

using CQuests = std::map<CQuest::ID, CQuest>;
