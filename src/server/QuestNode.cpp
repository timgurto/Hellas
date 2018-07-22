#include "QuestNode.h"
#include "Server.h"
#include "User.h"

void QuestNodeType::addQuestStart(const Quest::ID &id) {
  _questsStartingHere.insert(id);
}

bool QuestNodeType::startsQuest(const Quest::ID &id) const {
  return _questsStartingHere.find(id) != _questsStartingHere.end();
}

void QuestNodeType::addQuestEnd(const Quest::ID &id) {
  _questsEndingHere.insert(id);
}

bool QuestNodeType::endsQuest(const Quest::ID &id) const {
  return _questsEndingHere.find(id) != _questsEndingHere.end();
}
