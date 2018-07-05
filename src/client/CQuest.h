#pragma once

#include <map>
#include <string>

class Window;

class CQuest {
 public:
  using ID = std::string;
  using Name = std::string;
  using Prose = std::string;

  enum Transition { ACCEPT, COMPLETE };

  CQuest(const ID &id = "") : _id(id) {}

  bool operator<(const CQuest &rhs) { return _id < rhs._id; }

  const ID &id() const { return _id; }
  const Name &name() const { return _name; }
  void name(const Name &newName) { _name = newName; }
  const Prose &brief() const { return _brief; }
  void brief(const Prose &newBrief) { _brief = newBrief; }
  const Prose &debrief() const { return _debrief; }
  void debrief(const Prose &newDebrief) { _debrief = newDebrief; }
  const Window *window() const { return _window; }

  static void generateWindow(CQuest *quest, size_t startObjectSerial,
                             Transition pendingTransition);

  static void acceptQuest(CQuest *quest, size_t startObjectSerial);
  static void completeQuest(CQuest *quest, size_t startObjectSerial);

 private:
  ID _id;
  Name _name;
  Prose _brief, _debrief;
  Window *_window{nullptr};
};

using CQuests = std::map<CQuest::ID, CQuest>;
