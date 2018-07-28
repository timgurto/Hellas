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

void QuestNodeType::sendQuestsToUser(const User &user) const {
  const auto &server = Server::instance();

  for (const auto &id : _questsStartingHere) {
    auto quest = server.findQuest(id);
    if (quest->hasPrerequisite()) continue;
    if (user.hasCompletedQuest(id)) continue;
    server.sendMessage(user.socket(), SV_QUEST_CAN_BE_STARTED, id);
  }

  for (const auto &id : _questsEndingHere) {
    auto quest = server.findQuest(id);
    if (quest->hasObjective() && !user.hasKilledSomethingWhileOnQuest(id))
      continue;
    if (!user.isOnQuest(id)) continue;

    server.sendMessage(user.socket(), SV_QUEST_CAN_BE_FINISHED, id);
  }
}
