#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

class User;
class ServerItem;

struct Quest {
 public:
  using ID = std::string;
  ID id;
  std::set<std::string> startsWithItems{};
  int timeLimit = 0;  // in seconds.  0 = no time limit

  struct Objective {
    enum Type { NONE, KILL, FETCH, CONSTRUCT };
    Type type{NONE};
    static Type typeFromString(const std::string &asString);
    void setType(const std::string &asString);
    std::string typeAsString() const;

    std::string id{};
    const ServerItem *item{nullptr};
    int qty{1};
  };

  std::vector<Objective> objectives;
  bool hasObjective() const { return !objectives.empty(); }

  struct Reward {
    enum Type { NONE, CONSTRUCTION, SPELL };
    Type type{NONE};
    std::string id;
  };
  Reward reward;

  std::set<std::string> prerequisiteQuests{};
  bool hasPrerequisite() const { return !prerequisiteQuests.empty(); }
  std::set<std::string> otherQuestsWithThisAsPrerequisite;

  bool canBeCompletedByUser(const User &user) const;

  std::string exclusiveToClass{};
};

using Quests = std::map<Quest::ID, Quest>;
