#pragma once

#include <map>
#include <string>

class Window;

class CQuest {
 public:
  struct Info {
    using ID = std::string;
    using Name = std::string;
    using Prose = std::string;

    ID id;
    Name name;
    Prose brief, debrief;
    ID startsAt, endsAt;
  };

  enum Transition { ACCEPT, COMPLETE };

  CQuest(const Info &info = {}) : _info(info) {}

  bool operator<(const CQuest &rhs) { return _info.id < rhs._info.id; }

  const Info &info() const { return _info; }

  const Window *window() const { return _window; }

  static void generateWindow(CQuest *quest, size_t startObjectSerial,
                             Transition pendingTransition);

  static void acceptQuest(CQuest *quest, size_t startObjectSerial);
  static void completeQuest(CQuest *quest, size_t startObjectSerial);

  enum State { CAN_ACCEPT, CAN_COMPLETE };
  State state{CAN_ACCEPT};

 private:
  Info _info;
  Window *_window{nullptr};
};

using CQuests = std::map<CQuest::Info::ID, CQuest>;
