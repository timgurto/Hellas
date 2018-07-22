#pragma once

#include <set>

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

 private:
  SomeQuests _questsStartingHere{};
  SomeQuests _questsEndingHere{};
};

class QuestNode {
 protected:
  QuestNode(const QuestNodeType &type, size_t serial)
      : _type(&type), _serial(serial) {}
  static QuestNode Dummy() { return {}; }

 public:
  bool startsQuest(const Quest::ID &id) const { return _type->startsQuest(id); }
  bool endsQuest(const Quest::ID &id) const { return _type->endsQuest(id); }

 private:
  QuestNode() {}
  const QuestNodeType *_type{nullptr};
  size_t _serial{0};
};
