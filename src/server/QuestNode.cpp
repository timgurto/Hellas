#include "QuestNode.h"
#include "Server.h"
#include "User.h"

void QuestNode::sendQuestsToClient(const User &targetUser) const {
  const Server &server = Server::instance();

  auto questsToSend = questsUserCanStartHere(targetUser);
  auto args = makeArgs(_serial, questsToSend.size());
  for (auto questID : questsToSend) args = makeArgs(args, questID);
  server.sendMessage(targetUser.socket(), SV_OBJECT_STARTS_QUESTS, args);

  questsToSend = questsUserCanEndHere(targetUser);
  args = makeArgs(_serial, questsToSend.size());
  for (auto questID : questsToSend) args = makeArgs(args, questID);
  server.sendMessage(targetUser.socket(), SV_OBJECT_ENDS_QUESTS, args);
}

Quests QuestNode::questsUserCanStartHere(const User &user) const {
  auto ret = Quests{};

  for (const auto &questID : _type->questsStartingHere()) {
    if (user.isOnQuest(questID)) continue;
    ret.insert(questID);
  }

  return ret;
}

Quests QuestNode::questsUserCanEndHere(const User &user) const {
  auto ret = Quests{};

  for (const auto &questID : _type->questsStartingHere()) {
    if (!user.isOnQuest(questID)) continue;
    ret.insert(questID);
  }

  return ret;
}

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
