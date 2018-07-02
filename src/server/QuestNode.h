#pragma once

#include <set>

#include "Quest.h"

class User;

class QuestNodeType {
 public:
  using Quests = std::set<Quest::ID>;

  void addQuestStart(const Quest::ID &id);
  bool startsQuest(const Quest::ID &id) const;
  void addQuestEnd(const Quest::ID &id);
  bool endsQuest(const Quest::ID &id) const;
  const Quests &questsStartingHere() const { return _questsStartingHere; }

 private:
  Quests _questsStartingHere{};
  Quests _questsEndingHere{};
};

class QuestNode {
 protected:
  QuestNode(const QuestNodeType &type, size_t serial)
      : _type(&type), _serial(serial) {}
  static QuestNode Dummy() { return {}; }

 public:
  void sendQuestsToClient(const User &targetUser) const;
  bool startsQuest(const Quest::ID &id) const { return _type->startsQuest(id); }
  bool endsQuest(const Quest::ID &id) const { return _type->endsQuest(id); }

 private:
  QuestNode() {}
  const QuestNodeType *_type{nullptr};
  size_t _serial{0};
};
