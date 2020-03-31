#pragma once

#include <set>

#include "../Serial.h"
#include "Quest.h"

class User;

using SomeQuests = std::set<Quest::ID>;

class QuestNodeType {
 public:
  void addQuestStart(const Quest::ID &id);
  bool startsQuest(const Quest::ID &id) const;
  void addQuestEnd(const Quest::ID &id);
  bool endsQuest(const Quest::ID &id) const;
  const SomeQuests &questsStartingHere() const { return _questsStartingHere; }
  const SomeQuests &questsEndingHere() const { return _questsEndingHere; }
  void sendQuestsToUser(const User &user) const;

 private:
  SomeQuests _questsStartingHere{};
  SomeQuests _questsEndingHere{};
};

class QuestNode {
 protected:
  QuestNode(const QuestNodeType &type, Serial serial)
      : _type(&type), _serial(serial) {}
  static QuestNode Dummy() { return {}; }

 public:
  bool startsQuest(const Quest::ID &id) const { return _type->startsQuest(id); }
  bool endsQuest(const Quest::ID &id) const { return _type->endsQuest(id); }
  void sendQuestsToUser(const User &user) const {
    _type->sendQuestsToUser(user);
  };

 private:
  QuestNode() {}
  const QuestNodeType *_type{nullptr};
  Serial _serial;
};
