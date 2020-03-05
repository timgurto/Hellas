#pragma once

#include <map>
#include <string>
#include <vector>

#include "../types.h"

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

    struct Reward {
      enum Type { NONE, LEARN_SPELL, LEARN_CONSTRUCTION, RECEIVE_ITEM };
      Type type{NONE};
      std::string id{};
      int itemQuantity{1};
    };

    ID id;
    Name name;
    Prose brief, debrief;
    ID startsAt, endsAt;
    std::vector<Objective> objectives;
    Reward reward;
    std::string helpTopicOnAccept, helpTopicOnComplete;
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
  void setTimeRemaining(ms_t t) { _timeRemaining = t; }
  std::string nameInProgressUI() const;

  void update(ms_t timeElapsed);

 private:
  Info _info;
  Window *_window{nullptr};
  std::vector<int> _progress;  // objective index -> progress

  ms_t _timeRemaining{0};  // If >0, display.
  std::string _lastTimeDisplay{};
};

using CQuests = std::map<CQuest::Info::ID, CQuest>;
