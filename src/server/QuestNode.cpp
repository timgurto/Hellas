#include "QuestNode.h"
#include "Server.h"
#include "User.h"

void QuestNode::sendQuestsToClient(const User &targetUser) const {
  auto questsToSend = std::set<std::string>{};
  for (const auto &questID : _type->questsStartingHere())
    if (!targetUser.isOnQuest(questID)) questsToSend.insert(questID);
  if (questsToSend.empty()) return;

  auto args = makeArgs(_serial, questsToSend.size());
  for (auto questID : questsToSend) args = makeArgs(args, questID);

  const Server &server = Server::instance();
  server.sendMessage(targetUser.socket(), SV_OBJECT_GIVES_QUESTS, args);
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
