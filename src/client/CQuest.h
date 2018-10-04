#pragma once

#include <map>
#include <string>
#include <vector>

class Window;

class CQuest {
 public:
  struct Info {
    using ID = std::string;
    using Name = std::string;
    using Prose = std::string;

    // The client doesn't need to know what the objectives actually mean.
    struct Objective {
      std::string text;
      int qty;
    };

    ID id;
    Name name;
    Prose brief, debrief;
    ID startsAt, endsAt;
    std::vector<Objective> objectives;
    std::string helpTopicOnAccept;
  };

  enum Transition { ACCEPT, COMPLETE, INFO_ONLY };

  CQuest(const Info &info = {});

  bool operator<(const CQuest &rhs) { return _info.id < rhs._info.id; }

  const Info &info() const { return _info; }

  const Window *window() const { return _window; }

  static void generateWindow(CQuest *quest, size_t startObjectSerial,
                             Transition pendingTransition);

  static void acceptQuest(CQuest *quest, size_t startObjectSerial);
  static void completeQuest(CQuest *quest, size_t startObjectSerial);

  enum State { NONE, CAN_START, IN_PROGRESS, CAN_FINISH };
  State state{NONE};
  bool userIsOnQuest() const {
    return state == IN_PROGRESS || state == CAN_FINISH;
  }

  void setProgress(size_t objective, int progress);
  int getProgress(size_t objective) const;

 private:
  Info _info;
  Window *_window{nullptr};
  std::vector<int> _progress;  // objective index -> progress
};

using CQuests = std::map<CQuest::Info::ID, CQuest>;
